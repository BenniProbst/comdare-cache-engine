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

#include "profile.cpp"

#define trap(expr) if (!!(expr)) raise(SIGTRAP);

using namespace std;

static const unsigned maxStrLen=343;

double mydist(const char* primary, size_t pl, const char* secondary, size_t sl) {
    size_t matches = 0;
    size_t i = 0;
    for (; i < pl; ++i) {
        char c2 = 0;
        if (i < sl) { c2 = secondary[i]; }
        if (primary[i] != c2) {
            break;
        } else {
            matches++;
        }
    }

    const double alpha = 1./pl;
//float alpha = 1.;
//if (matches > 0) alpha = 1./matches;

cout.precision(std::numeric_limits<double>::max_digits10);

    cout << "alpha: " << alpha << endl;

    double dist = (1. - alpha)*static_cast<double>(pl - matches)/pl;
    double c1 = 0;
    if (i < pl) { c1 = primary[i]; };
    double c2 = 0;
    if (i < sl) { c2 = secondary[i]; }
    dist += alpha*std::abs(c1 - c2)/255.0;
    cout << "c1: " << c1 << " c2: " << c2 << " last: " << std::abs(c1/255. - c2/255.) << endl;
    return dist;
}

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

template<class T>
std::tuple<size_t, size_t> getBounds(T* node, char* key, unsigned len) {
    size_t count = std::min<size_t>(8ul, len);

    uint64_t snippet = 0;
    std::memcpy(&snippet, key, count);
    snippet = __builtin_bswap64(snippet);

    size_t lower = 0, upper = node->count;
    uint64_t prev = 0;
    size_t prevUpper = 0;
    for (size_t i = 0; i < T::intervalCount; ++i) {
       auto& hint = node->hints[i];

        if (prev < snippet && hint.snippet < snippet) {
            lower = prevUpper;
        }

        if (hint.snippet > snippet) {
#ifndef NDEBUG
//            std::cout << "compare with snippets[" << i << "]: " << std::hex << hint.snippet << std::dec << std::endl;
#endif
            upper = hint.upperBound;
            break;
        }

        prev = hint.snippet;
        prevUpper = hint.upperBound;
    }

    upper = std::min<size_t>(upper, node->count);

   return {lower, upper};
}

struct Node {
   static const unsigned nodeSize=56*1024;
   static const unsigned payloadSize=sizeof(Node*);
   static const unsigned intervalCount = 16;

   bool isLeaf;
   uint16_t count;
   uint16_t offset;
   uint16_t prefixLen;
   union {
      Node* upper; // inner
      Node* next;  // leaf
   };

   struct {
      uint64_t snippet;
      uint32_t upperBound;
//      char* key;
   } hints[intervalCount];

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

   inline Node* binarySearch(char* key,unsigned len, unsigned begin, unsigned end) {
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
      char* shortenedKey = key+prefixLen;
      unsigned shortenedLen = len-prefixLen;

      int prefixStatus=checkPrefix(key,len);
      if (!isLeaf) {
         switch (prefixStatus) {
            case -2: return false;//nullptr;
            case -1: return (*getValue(0))->lookup(key,len,ret);
            case +1: return upper->lookup(key,len,ret);
         }

         auto [lowerBound, upperBound] = getBounds(this, shortenedKey, shortenedLen);
         return binarySearch(shortenedKey, shortenedLen, lowerBound, upperBound)->lookup(key,len,ret);
      } else {
         if (prefixStatus!=0)
            return false;

         auto [lowerBound, upperBound] = getBounds(this, shortenedKey, shortenedLen);
         unsigned begin = lowerBound, end = upperBound;
#if 0
         while (begin<end) {
            unsigned mid=begin+((end-begin)/2);
            int c=cmp(shortenedKey, shortenedLen, getKey(mid),slots[mid].len);
            if (c==0) {
               ret=reinterpret_cast<uint64_t>(*getValue(mid));
               return true;
            }
            if (c<0)
               end=mid; else
               begin=mid+1;
         }
#else
auto n = end - begin;
auto lower = begin;
while (auto half=n/2) {
   auto middle=lower+half;
   __builtin_prefetch(&slots[middle]);
   int c = cmp(shortenedKey, shortenedLen, getKey(middle), slots[middle].len);
   lower=(c >= 0) ? middle : lower;
   n-=half;
}
   int c = cmp(shortenedKey, shortenedLen, getKey(lower), slots[lower].len);
if (c == 0) {
   ret=reinterpret_cast<uint64_t>(*getValue(lower));
   return true;
} else {
   return false;
}
#endif
         return false;
      }
   }



   void initSnippets() {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
      auto extractor = [&](char* str, size_t length) {
         uint64_t snippet = 0;
         char* snippetView = reinterpret_cast<char*>(&snippet);
//         size_t limit = std::min<size_t>(length - prefixLength, 8);
         size_t limit = std::min<size_t>(length, 8);
         for (size_t j = 0; j < limit; ++j) {
               size_t snippetIdx = 7 - j;
               snippetView[snippetIdx] = str[j];
         }
         return snippet;
      };
#else
#error "not implemented"
#endif

      size_t scale = std::max<size_t>(1, count/intervalCount);
      for (size_t i = 0; i < intervalCount - 1; ++i) {
         uint32_t idx = (i + 1)*scale - 1;
         idx = std::min<uint32_t>(idx, count - 1);
//         hints[i] = {extractor(getKey(idx), slots[idx].len), idx + 1, getKey(idx)};
         hints[i] = {extractor(getKey(idx), slots[idx].len), idx + 1};
      }
      uint32_t last = count - 1;
//      hints[intervalCount - 1] = {extractor(getKey(last), slots[last].len), last + 1, getKey(last)};
      hints[intervalCount - 1] = {extractor(getKey(last), slots[last].len), last + 1};

#ifndef NDEBUG
      for (size_t i = 0; i < intervalCount; ++i) {
//         std::cout << "snippets[" << i << "]: " << std::hex << hints[i].snippet << std::dec << std::endl;
      }
#endif
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

         initSnippets();
         return true;
      }

      unsigned newPrefixLength=commonPrefix(key,size);
      if (newPrefixLength==prefixLen) {
         if (!hasSpace(size))
            return false;
         // Same prefix
         add(key+prefixLen,size-prefixLen,value);

         initSnippets();
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
            trap(!node->lookup(keyInput, len, ret));
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
         assert(node->insert(k,len,value));
      }

      keyLeaf.push_back(node);
   }
   leaves.push_back(node);

   // 386MB->160MB
   cout << "countPerNode:" << n/leaves.size() << endl;

   Node* root = constructInnerNodes(leaves);
   return {root, keyLeaf};
}

void dumpDists(Node* node) {
   char* first = node->getKey(0);
   auto firstLen = node->slots[0].len;
   for (size_t i = 1; i < node->count; ++i) {
      char* current = node->getKey(i);
      auto currentLen = node->slots[i].len;
      double dist = mydist(first, firstLen, current, currentLen);
      printChars(node->getKey(i), node->slots[i].len); cout << endl;
      cout << "dist: " << dist << endl;
   }
}

int main(int argc,char** argv) {
   char* buffer=new char[1000000000];
   uint64_t pos=0;
   FILE* file=fopen(argv[1],"r");
   vector<char*> keys;
   while (fgets(buffer+pos, maxStrLen, file) != NULL) {
      size_t len = strlen(buffer+pos);
      assert(buffer[pos+len-1] == '\n');
      buffer[pos+len-1] = '\0';
      keys.push_back(buffer+pos);
      pos+=len;
   }

   sort(keys.begin(),keys.end(),[] (char *a, char *b) { return cmp(a,strlen(a),b,strlen(b))<0; });

   uint64_t n=keys.size();

   auto [root, keyLeaf] = constructTree(keys);
   cout << "root count: " << root->count << endl;
/*
   {
      Node* inner=root;
      for (unsigned i=0;i<inner->count;i++) {
         printChars(inner->getKey(i),inner->slots[i].len); cout << " ";
      } cout << endl;
   }
*/
dumpDists(root);
   {
      uint64_t ret;
      for (unsigned i=0; i<n; i++) {
          trap(!root->lookup(keys[i],strlen(keys[i]),ret));
         assert(root->lookup(keys[i],strlen(keys[i]),ret));
         assert(ret==i);
      }
   }

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

   timeAndProfile("pbtree leaflookup", n, [&]() {
         for (uint64_t i=0;i<n;i++) {
            uint64_t ret;
            rnd[i].leaf->lookup(rnd[i].key,strlen(rnd[i].key),ret);
            assert(ret==rnd[i].pos);
         }
      });
/*
   timeAndProfile("pbtree lookup", n, [&]() {
         for (uint64_t i=0;i<n;i++) {
            uint64_t ret;
            assert(root->lookup(rnd[i].key,strlen(rnd[i].key),ret));
            assert(ret==rnd[i].pos);
         }
      });
*/
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
