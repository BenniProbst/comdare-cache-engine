#include <iostream>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <sys/time.h>
#include <map>
#include <vector>
#include <csignal>

#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>

#include "profile.hpp"
#include "utils.hpp"

#define trap(expr) if (!!(expr)) raise(SIGTRAP);

using namespace std;

static const unsigned maxStrLen=512;

void printChars (char* str,unsigned len) {
   for (unsigned i=0;i<len;i++)
      cout << *(str++);
}

int cmp(char* k1,unsigned k1len,char* k2,unsigned k2len) {
   if (k1len==k2len)
      return memcmp(k1,k2,k1len);

   unsigned min=k1len<k2len?k1len:k2len;
   int c=memcmp(k1,k2,min);
   if (c!=0)
     return c;
   if (k1len<k2len)
      return -1;
   return +1;
}

struct Node {
   static const unsigned nodeSize=32*1024;
   static const unsigned payloadSize=sizeof(Node*);

   bool isLeaf;
   uint16_t count;
   uint16_t offset;
   uint16_t prefixLen;
   union {
      Node* upper; // inner
      Node* next;  // leaf
   };

   struct {
      uint16_t len;    // key length without prefix
      uint16_t offset; // location: ((char*)this)+offset
   } slots[1];

   Node(bool isLeaf) : isLeaf(isLeaf),count(0),offset(nodeSize),prefixLen(0) {}

   inline char* getPrefix() { return reinterpret_cast<char*>(this)+nodeSize-prefixLen; }
   inline Node** getValue(unsigned slot) { return reinterpret_cast<Node**>(getKey(slot)+slots[slot].len); }
   inline char* getKey(unsigned slot) { return reinterpret_cast<char*>(this)+slots[slot].offset; }

   unsigned combineKey(unsigned index,char* key) {
      memcpy(key,getPrefix(),prefixLen);
      memcpy(key+prefixLen,getKey(index),slots[index].len);
      return slots[index].len+prefixLen;
   }

   unsigned commonPrefix(char* b,unsigned len) {
      char* a=getPrefix();
      unsigned minLen=len<prefixLen?len:prefixLen;
      for (unsigned i=0;i<minLen;i++)
         if (a[i]!=b[i])
            return i;
      return minLen;
   }

   bool hasSpace(unsigned size) { return ((reinterpret_cast<char*>(this)+offset-size-payloadSize)>reinterpret_cast<char*>(&slots[count+1])); }

   inline int checkPrefix(char* key,unsigned len) {
      char* prefix=getPrefix();
      for (unsigned i=0;i<prefixLen;i++) {
         if (i==len)
            // Key too short
            return -2;
         if (key[i]<prefix[i])
            // Key less
            return -1;
         if (key[i]>prefix[i])
            // Key larger
            return 1;
      }
      return 0;
   }

   inline Node* binarySearch(char* key,unsigned len) {
      unsigned begin=0,end=count;

      while (true) {
         if (end-begin<4) {
            for (unsigned pos=begin;pos<end;pos++)
               if (cmp(key,len,getKey(pos),slots[pos].len)<=0)
                  return *getValue(pos);
            return upper;
         } else {
            unsigned mid=begin+((end-begin)/2);
            int c=cmp(key,len,getKey(mid),slots[mid].len);
            if (c>0)
               begin=mid+1; else
               end=mid+1;
         }
      }
   }

   bool lookup(char* key,unsigned len,uint64_t& ret) {
      int prefixStatus=checkPrefix(key,len);
      if (!isLeaf) {
         switch (prefixStatus) {
            case -2: return false;//nullptr;
            case -1: return (*getValue(0))->lookup(key,len,ret);
            case +1: return upper->lookup(key,len,ret);
         }

         return binarySearch(key+prefixLen,len-prefixLen)->lookup(key,len,ret);
      } else {
         if (prefixStatus!=0)
            return false;

         char* k=key+prefixLen;
         unsigned l=len-prefixLen;
         unsigned begin=0,end=count;
         while (begin<end) {
            unsigned mid=begin+((end-begin)/2);
            int c=cmp(k,l,getKey(mid),slots[mid].len);
            if (c==0) {
               ret=reinterpret_cast<uint64_t>(*getValue(mid));
               return true;
            }
            if (c<0)
               end=mid; else
               begin=mid+1;
         }
         return false;
      }
   }

   void add(char* key,unsigned size,Node* value) {
      // add a single entry
      assert(hasSpace(size));
      offset-=size+payloadSize;
      slots[count].len=size;
      slots[count].offset=offset;
      memcpy(getKey(count),key,size);
      *getValue(count)=value;
      count++;
   }

   unsigned minKey(char* k) {
      if (isLeaf)
         return combineKey(0,k);

      return (*getValue(0))->minKey(k);
   }

   unsigned maxKey(char* k) {
      if (isLeaf)
         return combineKey(count-1,k);

      return upper->maxKey(k);
   }

   bool insert(char* key,unsigned size,Node* value) {
      if (count==0) {
         // Key becomes prefix
         prefixLen=size;
         offset-=size;
         memcpy(getPrefix(),key,size);
         // Store empty (remaining) key and value
         add(key,0,value);
         return true;
      }

      unsigned newPrefixLength=commonPrefix(key,size);
      if (newPrefixLength==prefixLen) {
         if (!hasSpace(size))
            return false;
         // Same prefix
         add(key+prefixLen,size-prefixLen,value);
         return true;
      }

      assert(prefixLen>newPrefixLength);
      // Is there enough space with a shorter prefix?
      unsigned additionalSpace=size+payloadSize+count*(prefixLen-newPrefixLength);
      if ((reinterpret_cast<char*>(this)+offset-additionalSpace)<reinterpret_cast<char*>(&slots[count+1]))
         return false;

      // Enough space, grow all entries
      char scratchSpace[nodeSize];
      auto newNode=new (reinterpret_cast<Node*>(scratchSpace)) Node(isLeaf);
      newNode->prefixLen=newPrefixLength;
      newNode->count=count;

      // Set new prefix
      newNode->offset-=newPrefixLength;
      memcpy(newNode->getPrefix(),getPrefix(),newPrefixLength);

      // Copy slots and keys
      unsigned prefixDiff=prefixLen-newPrefixLength;
      for (unsigned i=0;i<count;i++) {
         newNode->offset-=slots[i].len+prefixDiff+payloadSize;
         newNode->slots[i].len=slots[i].len+prefixDiff;
         newNode->slots[i].offset=newNode->offset;
         memcpy(scratchSpace+newNode->offset,getPrefix()+newPrefixLength,prefixDiff);
         memcpy(scratchSpace+newNode->offset+prefixDiff,getKey(i),slots[i].len+payloadSize);
         assert(*getValue(i) == *newNode->getValue(i));
      }

      if (newNode->insert(key, size, value)) {
         memcpy(this,newNode,nodeSize);
         return true;
      }
      return false;
   }

   static Node* makeLeaf() { return new (static_cast<Node*>(malloc(nodeSize))) Node(true); }
   static Node* makeInner() { return new (static_cast<Node*>(malloc(nodeSize))) Node(false); }
};

struct cmp_str { bool operator()(char const *a, char const *b) { return std::strcmp(a, b)<0; } };

unsigned truncateSeparator(char* lower,unsigned lowerLen,char* upper,unsigned upperLen,char* k) {
   unsigned minLen=lowerLen<upperLen?lowerLen:upperLen;
   unsigned commonPrefix;
   for (commonPrefix=0; commonPrefix<minLen; commonPrefix++)
      if (lower[commonPrefix]!=upper[commonPrefix])
         break;
   if (commonPrefix==lowerLen) {
      memcpy(k,lower,lowerLen);
      return lowerLen;
   }
   if (lower[commonPrefix]<upper[commonPrefix]&&(lower[commonPrefix]+1<upper[commonPrefix])) {
      memcpy(k,lower,commonPrefix+1);
      k[commonPrefix]++;
      return commonPrefix+1;
   }
   memcpy(k,lower,lowerLen);
   return lowerLen;
}

Node* constructInnerNodes(vector<Node*> lowerLevel) {
   if (lowerLevel.size() == 1) {
      return lowerLevel.front();
   }

   vector<Node*> currentLevel;
   Node* node=Node::makeInner();
   for (unsigned i=0; i<lowerLevel.size()-1; i++) {
      Node* curr=lowerLevel[i];
      Node* next=lowerLevel[i+1];
      char lower[maxStrLen]; unsigned lowerLen=curr->maxKey(lower);
      char upper[maxStrLen]; unsigned upperLen=next->minKey(upper);
      char k[maxStrLen]; unsigned len=truncateSeparator(lower,lowerLen,upper,upperLen,k);
      assert(cmp(lower,lowerLen,k,len)<=0);

      if (!node->insert(k,len,curr)) {
         node->upper=curr;
         currentLevel.push_back(node);
         node=Node::makeInner();
      }
   }
   node->upper=lowerLevel[lowerLevel.size()-1];
   currentLevel.push_back(node);
   cout << "countPerNode:" << lowerLevel.size()/currentLevel.size() << endl;

   return constructInnerNodes(currentLevel);
}

std::tuple<Node*, std::vector<Node*>> constructTree(const vector<char*>& keys) {
   uint64_t n=keys.size();

   vector<Node*> leaves;
   vector<Node*> keyLeaf;
   Node* node=Node::makeLeaf();
   for (uint64_t i=0;i<n;i++) {
      char* k=keys[i];
      unsigned len=strlen(keys[i]);
      Node* value=(Node*)i;
      if (!node->insert(k,len,value)) {
//{ validation:
         for (size_t j = 0; j < node->count; ++j) {
            size_t keyArrayIdx = i - node->count + j;
            char* keyInput = keys[keyArrayIdx];
            unsigned len = strlen(keyInput);
            char keyStored[maxStrLen];
            node->combineKey(j, keyStored);
            bool match = cmp(keyInput, len, keyStored, len) == 0;
            if (!match) {
               cout << "expected: "; printChars(keyInput, len);
               cout << " got: "; printChars(keyStored, len); cout << endl;
               assert(match);
            }

            uint64_t ret;
            assert(node->lookup(keyInput, len, ret));
            assert(ret == keyArrayIdx);
         }
/*
         char maxKey[maxStrLen]; unsigned maxKeyLen=node->maxKey(maxKey);
         char minKey[maxStrLen]; unsigned minKeyLen=node->minKey(minKey);
         cout << "minKey: "; printChars(minKey, minKeyLen); cout << endl;
         cout << "maxKey: "; printChars(maxKey, maxKeyLen); cout << endl;
*/
//}
         leaves.push_back(node);
         node=Node::makeLeaf();
         bool inserted = node->insert(k,len,value);
         assert(inserted);
      }

      keyLeaf.push_back(node);
   }
   leaves.push_back(node);

   // 386MB->160MB
   cout << "countPerNode:" << n/leaves.size() << endl;

   Node* root = constructInnerNodes(leaves);
   return {root, keyLeaf};
}

int main(int argc,char** argv) {
   vector<char*> keys;
   char* buffvar=nullptr;
   size_t len=0;
   FILE* file=fopen(argv[1],"r");
   while (getline(&buffvar, &len, file)!=-1) {
      keys.push_back(buffvar);
      buffvar=nullptr;
   }

   sort(keys.begin(),keys.end(),[] (char *a, char *b) { return cmp(a,strlen(a),b,strlen(b))<0; });

   uint64_t n=keys.size();
/*
   auto [root, keyLeaf] = constructTree(keys);
   Node* rootPtr = root;
   */
   auto r = constructTree(keys);
   Node* root = get<0>(r);
   std::vector<Node*>& keyLeaf = get<1>(r);

#ifndef NDEBUG
   cout << "root count: " << root->count << endl;
   {
      Node* inner=root;
      for (unsigned i=0;i<inner->count;i++) {
         printChars(inner->getKey(i),inner->slots[i].len); cout << " ";
      } cout << endl;
   }

   {
      uint64_t ret;
      for (unsigned i=0; i<n; i++) {
         assert(root->lookup(keys[i],strlen(keys[i]),ret));
         assert(ret==i);
      }
   }
#endif

   struct RND {
      char* key;
      Node* leaf;
      unsigned pos;
   };

   RND* rnd=new RND[n];
   for (uint64_t i=0;i<n;i++) {
      rnd[i].key=keys[i];
      rnd[i].leaf=keyLeaf[i];
      rnd[i].pos=i;
   }
   std::random_shuffle(rnd,rnd+n);

   size_t repetitions = 1;
   char* repetitionsEnv = getenv("REPETITIONS");
   if (repetitionsEnv) {
      repetitions = std::atoi(repetitionsEnv);
   }

   string dataset("'");
   dataset += getFileName(argv[1]);
   dataset += "'";

   PerfEvents e;
   {
      e.timeAndProfile("leafLookup", n, [&]() {
               for (uint64_t i=0;i<n;i++) {
                  uint64_t ret;
                  rnd[i].leaf->lookup(rnd[i].key,strlen(rnd[i].key),ret);
                  assert(ret==rnd[i].pos);
               }
         }, repetitions, {{"approach", "pbtree"}, {"dataset", dataset}});
   }

   {
      e.timeAndProfile("lookup", n, [&]() {
               for (uint64_t i=0;i<n;i++) {
                  uint64_t ret;
                  bool found = root->lookup(rnd[i].key,strlen(rnd[i].key),ret);
                  assert(found);
                  assert(ret==rnd[i].pos);
//                  if (ret!=rnd[i].pos) throw 0;
               }
         }, repetitions, {{"approach", "pbtree"}, {"dataset", dataset}});
   }

   return 0;
}

/*
6.4M URLS
maxlength 343
avg length 62
space 386MB

prefix btree (full 9KB nodes):
lookup=0.75M/s
cachemisses=30
height=3
space=165MB

ART:
lookup=1.3M/s
cachemisses=14
height=18 (avg)
height=31 (max)
space=158MB+386MB
N4: 9.6228 N16: 6.57854 N48: 1.9264 N256: 0.0749896

STL map:
lookup=0.44M/s
cachemisses=58
 */
