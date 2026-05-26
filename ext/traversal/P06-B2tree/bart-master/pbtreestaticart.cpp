#include <iostream>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <sys/time.h>
#include <map>
#include <cstddef>
#include <x86intrin.h>
#include <vector>

#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>

using namespace std;

class PerfEvent {
   vector<struct perf_event_attr> pes;
   vector<int> fd;
   unsigned cpucount;

public:
   PerfEvent(uint64_t type, uint64_t event) {
      cpucount=sysconf(_SC_NPROCESSORS_ONLN);
      pes.reserve(cpucount);
      fd.reserve(cpucount);

      for (unsigned i=0; i<cpucount; i++) {
         struct perf_event_attr& pe=pes[i];
         memset(&pe, 0, sizeof(struct perf_event_attr));
         pe.type = type;
         pe.size = sizeof(struct perf_event_attr);
         pe.config = event;
         pe.disabled = true;
         pe.exclude_kernel = true;
         pe.exclude_hv = true;

         fd[i] = syscall( __NR_perf_event_open, &pe, -1, i, -1, 0);

         if (fd[i] < 0) {
            fprintf(stderr, "Error opening leader %llx\n", pe.config);
         }
      }

      for (unsigned i=0; i<cpucount; i++) {
         ioctl(fd[i], PERF_EVENT_IOC_RESET, 0);
         ioctl(fd[i], PERF_EVENT_IOC_ENABLE, 0);
      }
   }

   ~PerfEvent() {
      for (unsigned i=0; i<cpucount; i++)
         close(fd[i]);
   }

   uint64_t readCounter() {
      uint64_t sum=0;
      for (unsigned i=0; i<cpucount; i++) {
         uint64_t count;
         if (read(fd[i], &count, sizeof(uint64_t))!=sizeof(uint64_t))
            fprintf(stderr, "Error reading counter\n");
         sum+=count;
      }
      return sum;
   }
};

static const unsigned maxStrLen=343;

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
   static const unsigned nodeSize=9*1024;
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
   } slots[0];

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
            case -2: return false;
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
      }
      memcpy(this,newNode,nodeSize);

      return insert(key,size,value);
   }

   static Node* makeLeaf() { return new (static_cast<Node*>(malloc(nodeSize))) Node(true); }
   static Node* makeInner() { return new (static_cast<Node*>(malloc(nodeSize))) Node(false); }
};

class ARTNode;

struct  __attribute__ ((__packed__)) ARTNode {
   uint8_t count;
   uint8_t skip;
};

struct __attribute__ ((__packed__)) NodeSpan : public ARTNode {
   //count=1
   uint16_t spanCount;
   char key[0];
};

struct __attribute__ ((__packed__)) Node2 : public ARTNode {
   //count==2
   char key[2];
   uint16_t child[2];
};

struct __attribute__ ((__packed__)) Node4 : public ARTNode {
   //count<5
   char key[4];
   uint16_t child[4];
};

struct __attribute__ ((__packed__)) Node16 : public ARTNode {
   //count<17
   char key[16];
   uint16_t child[16];
};

struct __attribute__ ((__packed__)) Node256 : public ARTNode {
   //count>16
   uint16_t child[256];
};

struct  __attribute__ ((__packed__)) NodeLeaf : public ARTNode {
   //count=0
   ARTNode* value;
   uint16_t len;
   char key[0];
};

struct NewNode {
   static const unsigned nodeSize=16*1024;
   static const unsigned payloadSize=sizeof(NewNode*);

   bool isLeaf;
   uint16_t count;
   uint16_t root;
   unsigned offset;
   union {
      NewNode* upper; // inner
      NewNode* next;  // leaf
   };
   char data;

   NewNode(bool isLeaf) : isLeaf(isLeaf),count(0),offset(offsetof(struct NewNode,data)) {}

   inline char* getData() { return reinterpret_cast<char*>(this)+offset; }
   static NewNode* makeLeaf() { return new (static_cast<NewNode*>(malloc(nodeSize))) NewNode(true); }
   static NewNode* makeInner() { return new (static_cast<NewNode*>(malloc(nodeSize))) NewNode(false); }

   void checkNode(uint16_t curr,unsigned& count,unsigned depth=0) {
      assert(curr);
      assert(curr<nodeSize);
      ARTNode* n=reinterpret_cast<ARTNode*>(reinterpret_cast<char*>(this)+curr);
      if (n->count==0) {
         count++;
         return;
      }

      if (n->count==1) {
         auto node=reinterpret_cast<NodeSpan*>(n);
         return checkNode(curr+sizeof(NodeSpan)+node->spanCount,count,depth+node->spanCount);
      }

      depth+=n->skip;

      if (n->count==2) {
         auto node=reinterpret_cast<Node2*>(n);
         for (unsigned i=0; i<n->count; i++)
            checkNode(node->child[i],count,depth+1);
         return;
      }

      if (n->count<5) {
         auto node=reinterpret_cast<Node4*>(n);
         for (unsigned i=0; i<n->count; i++)
            checkNode(node->child[i],count,depth+1);
         return;
      }

      if (n->count<17) {
         auto node=reinterpret_cast<Node16*>(n);
         for (unsigned i=0; i<n->count; i++)
            checkNode(node->child[i],count,depth+1);
         return;
      }

      auto node=reinterpret_cast<Node256*>(n);
      for (unsigned i=0; i<256; i++)
         if (node->child[i])
            checkNode(node->child[i],count,depth+1);
   }


   bool lookup(char* key,unsigned len,uint64_t& ret,uint16_t curr,unsigned depth=0) {
      ARTNode* n=reinterpret_cast<ARTNode*>(reinterpret_cast<char*>(this)+curr);
      if (n->count==0) {
         auto node=reinterpret_cast<NodeLeaf*>(n);
         if (memcmp(node->key,key+(len-node->len),node->len)!=0)
            return false;
         ret=reinterpret_cast<uint64_t>(node->value);
         return true;
      }

      if (n->count==1) {
         auto node=reinterpret_cast<NodeSpan*>(n);
         if (memcmp(node->key,key+depth,node->spanCount)!=0)
            return false;
         return lookup(key,len,ret,curr+sizeof(NodeSpan)+node->spanCount,depth+node->spanCount);
      }

      depth+=n->skip;
      char byte=key[depth];

      if (n->count==2) {
         auto node=reinterpret_cast<Node2*>(n);
         if (node->key[0]==byte)
            return lookup(key,len,ret,node->child[0],depth+1);
         if (node->key[1]==byte)
            return lookup(key,len,ret,node->child[1],depth+1);
         return false;
      }

      if (n->count<5) {
         auto node=reinterpret_cast<Node4*>(n);

         __m128i cmp=_mm_cmpeq_epi8(_mm_set1_epi8(byte),_mm_loadu_si128/*_mm_lddqu_si128*/(reinterpret_cast<__m128i*>(node->key)));
         unsigned bitfield=_mm_movemask_epi8(cmp)&((1<<n->count)-1);
         if (bitfield)
            return lookup(key,len,ret,node->child[__builtin_ctz(bitfield)],depth+1); else
            return false;
      }

      if (n->count<17) {
         auto node=reinterpret_cast<Node16*>(n);

         __m128i cmp=_mm_cmpeq_epi8(_mm_set1_epi8(byte),_mm_loadu_si128/*_mm_lddqu_si128*/(reinterpret_cast<__m128i*>(node->key)));
         unsigned bitfield=_mm_movemask_epi8(cmp)&((1<<n->count)-1);
         if (bitfield)
            return lookup(key,len,ret,node->child[__builtin_ctz(bitfield)],depth+1); else
            return false;
      }

      auto node=reinterpret_cast<Node256*>(n);
      unsigned child=node->child[reinterpret_cast<unsigned char*>(key)[depth]];
      if (child)
         return lookup(key,len,ret,child,depth+1);
      return false;
   }
};

unsigned countDistinct(Node* src,unsigned begin,unsigned end,unsigned depth) {
   unsigned countDistinct=1;
   for (unsigned i=begin+1;i<end;i++)
      if (src->getKey(i)[depth]!=src->getKey(i-1)[depth])
         countDistinct++;
   return countDistinct;
}

uint16_t transform(Node* src,NewNode* dst,unsigned begin,unsigned end,unsigned depth);

template<class T> void buildGroups(Node* src,NewNode* dst,unsigned begin,unsigned end,unsigned depth,ARTNode* n,unsigned c) {
   auto node=reinterpret_cast<T*>(n);
   dst->offset+=sizeof(T);
   // recurse for groups with equal keys
   unsigned groupStart=begin;
   unsigned childPos=0;
   for (unsigned i=begin+1; i<end; i++) {
      if (src->getKey(i)[depth]!=src->getKey(i-1)[depth]) {
         node->key[childPos]=src->getKey(i-1)[depth];
         node->child[childPos]=transform(src,dst,groupStart,i,depth+1);
         childPos++;
         groupStart=i;
      }
   }
   // last group
   node->key[childPos]=src->getKey(end-1)[depth];
   node->child[childPos]=transform(src,dst,groupStart,end,depth+1);
}

uint16_t transform(Node* src,NewNode* dst,unsigned begin,unsigned end,unsigned depth) {
   assert(end>begin);
   uint16_t offset=dst->offset;
   assert(offset<dst->nodeSize);
   assert(offset);

   assert(end>begin);

   if (end-begin==1) {
      // Leaf
      auto leaf=reinterpret_cast<NodeLeaf*>(dst->getData());
      leaf->count=0;
      leaf->value=(ARTNode*)(*src->getValue(begin));
      leaf->len=src->slots[begin].len;
      memcpy(leaf->key,src->getKey(begin),leaf->len);
      dst->offset+=sizeof(NodeLeaf)+leaf->len;
      assert(offset);
      return offset;
   }

   // Determine skip
   unsigned skip=0;
   unsigned c=countDistinct(src,begin,end,depth);
   while (c==1) {
      skip++;
      depth++;
      c=countDistinct(src,begin,end,depth);
   }
   assert(c<256);
   // Node
   auto n=reinterpret_cast<ARTNode*>(dst->getData());
   n->count=c;
   n->skip=skip;

   if (c==2) { buildGroups<Node2>(src,dst,begin,end,depth,n,c); }
   else if (c<5) { buildGroups<Node4>(src,dst,begin,end,depth,n,c); }
   else if (c<17) { buildGroups<Node16>(src,dst,begin,end,depth,n,c); }
   else {
      auto node=reinterpret_cast<Node256*>(n);
      dst->offset+=sizeof(Node256);
      memset(node->child,0,sizeof(node->child));
      // recurse for groups with equal keys
      unsigned groupStart=begin;
      for (unsigned i=begin+1; i<end; i++) {
         if (src->getKey(i)[depth]!=src->getKey(i-1)[depth]) {
            node->child[((unsigned char*)src->getKey(i-1))[depth]]=transform(src,dst,groupStart,i,depth+1);
            groupStart=i;
         }
      }
      // last group
      node->child[((unsigned char*)src->getKey(end-1))[depth]]=transform(src,dst,groupStart,end,depth+1);
   }

   assert(offset<dst->nodeSize);
   assert(offset);
   return offset;
}

NewNode* transform(Node* src) {
   NewNode* dst=NewNode::makeLeaf();
   dst->isLeaf=src->isLeaf;
   dst->count=src->count;
   dst->root=dst->offset;

   // NodeSpan for common prefix, followed by root node
   auto span=reinterpret_cast<NodeSpan*>(dst->getData());
   span->count=1;
   span->skip=0;
   span->spanCount=src->prefixLen;
   memcpy(span->key,src->getPrefix(),src->prefixLen);
   dst->offset+=sizeof(NodeSpan)+src->prefixLen;

   transform(src,dst,0,src->count,0);

   unsigned count=0;
   dst->checkNode(dst->root,count);
   assert(count==dst->count);
   return dst;
}

static inline double gettime(void) {
   struct timeval now_tv;
   gettimeofday (&now_tv, NULL);
   return ((double)now_tv.tv_sec) + ((double)now_tv.tv_usec)/1000000.0;
}

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

int main(int argc,char** argv) {
   char* buffer=new char[1000000000];
   uint64_t pos=0;
   FILE* file=fopen(argv[1],"r");
   vector<char*> keys;
   while (fscanf(file,"%s",buffer+pos)!=EOF) {
      keys.push_back(buffer+pos);
      pos+=strlen(buffer+pos)+2;
   }
   sort(keys.begin(),keys.end(),[] (char *a, char *b) { return cmp(a,strlen(a),b,strlen(b))<0; });
   uint64_t n=keys.size();

   for (unsigned i=1; i<n; i++) {
      unsigned l1=strlen(keys[i-1]);
      if (l1<strlen(keys[i])&&(memcmp(keys[i-1],keys[i],l1)==0)) {
         keys[i-1][l1]='\1';
         keys[i-1][l1+1]='\0';
      }
   }

   vector<Node*> leaves;
   vector<Node*> keyLeaf;
   Node* node=Node::makeLeaf();
   for (uint64_t i=0;i<n;i++) {
      char* k=keys[i];
      unsigned len=strlen(keys[i]);
      Node* value=(Node*)i;
      if (!node->insert(k,len,value)) {
         leaves.push_back(node);
         node=Node::makeLeaf();
         assert(node->insert(k,len,value));
      }
      keyLeaf.push_back(node);
   }
   leaves.push_back(node);

   // 386MB->160MB
   cout << "countPerNode:" << n/leaves.size() << endl;

/*
   {
      vector<NewNode*> artleaves;
      for (Node* n : leaves)
         artleaves.push_back(transform(n));
      unsigned sum=0;
      unsigned m=0;
      for (NewNode* n : artleaves) {
         if (n->offset>m)
            m=n->offset;
         sum+=n->offset;
      }
      cout << "avg:" << (float)sum/artleaves.size() << " max:" << m << endl;
   }
*/

   vector<Node*> inner1;
   node=Node::makeInner();
   for (unsigned i=0; i<leaves.size()-1; i++) {
      Node* curr=leaves[i];
      Node* next=leaves[i+1];
      char lower[maxStrLen]; unsigned lowerLen=curr->maxKey(lower);
      char upper[maxStrLen]; unsigned upperLen=next->minKey(upper);
      char k[maxStrLen]; unsigned len=truncateSeparator(lower,lowerLen,upper,upperLen,k);
      assert(cmp(lower,lowerLen,k,len)<=0);

      if (!node->insert(k,len,curr)) {
         node->upper=curr;
         inner1.push_back(node);
         node=Node::makeInner();
      }
   }
   node->upper=leaves[leaves.size()-1];
   inner1.push_back(node);
   cout << "countPerNode:" << leaves.size()/inner1.size() << endl;


   Node* root=Node::makeInner();
   for (unsigned i=0; i<inner1.size()-1; i++) {
      Node* curr=inner1[i];
      Node* next=inner1[i+1];
      char lower[maxStrLen]; unsigned lowerLen=curr->maxKey(lower);
      char upper[maxStrLen]; unsigned upperLen=next->minKey(upper);
      char k[maxStrLen]; unsigned len=truncateSeparator(lower,lowerLen,upper,upperLen,k);
      //printChars(lower,lowerLen); cout << " "; printChars(upper,upperLen); cout << " "; printChars(k,len); cout << endl;
      assert(root->insert(k,len,curr));
   }
   root->upper=inner1[inner1.size()-1];
   cout << "rootcount:" << root->count << " rootPrefixLen:" << root->prefixLen << endl;

   cout << "space MB:" << ((leaves.size()+inner1.size()+1)*Node::nodeSize)/1024/1024 << endl;

/*
   {
      Node* inner=root;
      for (unsigned i=0;i<inner->count;i++) {
         printChars(inner->getKey(i),inner->slots[i].len); cout << " ";
      } cout << endl;
   }
*/

/*
   {
      Node* inner=inner1[10];
      for (unsigned i=0;i<inner->count;i++) {
         printChars(inner->getKey(i),inner->slots[i].len); cout << " ";
      } cout << endl;

      // First char info
      for (unsigned j=10; j<30; j++){
         Node* inner=inner1[j];
         unsigned count[256];
         memset(count,0,sizeof(count));
         for (unsigned i=0;i<inner->count;i++)
            count[((unsigned char*)inner->getKey(i))[0]]++;
         cout << inner->count << "; ";
         for (auto c : count)
            if (c)
               cout << c << " ";

         cout << "; ";
         {
            unsigned max=0,maxPos=0;
            for (unsigned i=0;i<256;i++)
               if (count[i]>max) {
                  max=count[i];
                  maxPos=i;
               }
            unsigned count2[256];
            memset(count2,0,sizeof(count2));
            for (unsigned i=0;i<inner->count;i++)
               if (((unsigned char*)inner->getKey(i))[0]==maxPos)
                  count2[((unsigned char*)inner->getKey(i))[1]]++;
            unsigned countNotZero=0;
            for (auto c : count2) {
               if (c) {
                  cout << c << " ";
                  countNotZero++;
               }
            }
            cout << " (" <<countNotZero<< ")" << " ";
         }
         cout << endl;
      } cout << endl;

      for (unsigned j=10; j<30; j++){
         Node* inner=leaves[j];
         unsigned count[256];
         memset(count,0,sizeof(count));
         for (unsigned i=0;i<inner->count;i++)
            count[((unsigned char*)inner->getKey(i))[0]]++;
         cout << inner->count << "; ";
         for (auto c : count)
            if (c)
               cout << c << " ";

         cout << "; ";
         {
            unsigned max=0,maxPos=0;
            for (unsigned i=0;i<256;i++)
               if (count[i]>max) {
                  max=count[i];
                  maxPos=i;
               }
            unsigned count2[256];
            memset(count2,0,sizeof(count2));
            for (unsigned i=0;i<inner->count;i++)
               if (((unsigned char*)inner->getKey(i))[0]==maxPos)
                  count2[((unsigned char*)inner->getKey(i))[1]]++;
            for (auto c : count2)
               if (c)
                  cout << c << " ";
         }
         cout << endl;
      }
   }
*/

   {
      // Check btree
      unsigned j=0;
      for (unsigned y=0; y<=root->count; y++) {
         Node* inner;
         if (y==root->count)
            inner=root->upper; else
            inner=*root->getValue(y);

         char sepr[maxStrLen]; unsigned seprlen=(y==root->count)?0:root->combineKey(y,sepr);

         for (unsigned z=0; z<=inner->count; z++) {
            Node* n;
            if (z==inner->count)
               n=inner->upper; else
               n=*inner->getValue(z);

            char sep1[maxStrLen]; unsigned sep1len=(z==inner->count)?0:inner->combineKey(z,sep1);

            if (seprlen&&sep1len)
               assert(cmp(sep1,sep1len,sepr,seprlen)<=0);

            for (unsigned i=0; i<n->count; i++) {
               if (j==34174)
                  cout << y << " " << z << " " << i << endl;
               char k[maxStrLen]; unsigned len=n->combineKey(i,k);
               assert(cmp(k,len,keys[j],strlen(keys[j]))==0);
               if (sep1len)
                  assert(cmp(keys[j],strlen(keys[j]),sep1,sep1len)<=0);
               if (seprlen)
                  assert(cmp(keys[j],strlen(keys[j]),sepr,seprlen)<=0);
               j++;
            }
         }
      }
      assert(j==n);
   }

   {
      uint64_t ret;
      for (unsigned i=0; i<n; i++) {
         assert(root->lookup(keys[i],strlen(keys[i]),ret));
         assert(ret==i);
      }
   }

   struct RND {
      char* key;
      Node* leaf;
      NewNode* artleaf;
      unsigned pos;
   };

   RND* rnd=new RND[n];
   std::map<Node*,NewNode*> nodemap;
   for (uint64_t i=0;i<n;i++) {
      rnd[i].key=keys[i];
      rnd[i].leaf=keyLeaf[i];
      if (!nodemap[keyLeaf[i]])
         nodemap[keyLeaf[i]]=transform(keyLeaf[i]);
      rnd[i].artleaf=nodemap[keyLeaf[i]];
      rnd[i].pos=i;
   }
   std::random_shuffle(rnd,rnd+n);

   {
      PerfEvent misses(PERF_TYPE_HARDWARE,PERF_COUNT_HW_CACHE_MISSES);
      PerfEvent branches(PERF_TYPE_HARDWARE,PERF_COUNT_HW_BRANCH_INSTRUCTIONS);
      PerfEvent branchMisses(PERF_TYPE_HARDWARE,PERF_COUNT_HW_BRANCH_MISSES);
      double start = gettime();
      start = gettime();
      for (uint64_t i=0;i<n;i++) {
         uint64_t ret;
         assert(root->lookup(rnd[i].key,strlen(rnd[i].key),ret));
         assert(ret==rnd[i].pos);
      }
      printf("lookup %f %f %f %f\n",(n/1000000.0)/(gettime()-start),misses.readCounter()/(float)n, branches.readCounter()/(float)n, branchMisses.readCounter()/(float)n);
   }


   {
      PerfEvent misses(PERF_TYPE_HARDWARE,PERF_COUNT_HW_CACHE_MISSES);
      PerfEvent branches(PERF_TYPE_HARDWARE,PERF_COUNT_HW_BRANCH_INSTRUCTIONS);
      PerfEvent branchMisses(PERF_TYPE_HARDWARE,PERF_COUNT_HW_BRANCH_MISSES);
      double start = gettime();
      start = gettime();
      for (uint64_t i=0;i<n;i++) {
         uint64_t ret;
         rnd[i].leaf->lookup(rnd[i].key,strlen(rnd[i].key),ret);
         assert(ret==rnd[i].pos);
      }
      printf("leaflookup %f %f %f %f\n",(n/1000000.0)/(gettime()-start),misses.readCounter()/(float)n, branches.readCounter()/(float)n, branchMisses.readCounter()/(float)n);
   }

   {
      PerfEvent misses(PERF_TYPE_HARDWARE,PERF_COUNT_HW_CACHE_MISSES);
      PerfEvent branches(PERF_TYPE_HARDWARE,PERF_COUNT_HW_BRANCH_INSTRUCTIONS);
      PerfEvent branchMisses(PERF_TYPE_HARDWARE,PERF_COUNT_HW_BRANCH_MISSES);
      double start = gettime();
      start = gettime();
      for (uint64_t i=0;i<n;i++) {
         __builtin_prefetch(((char*)rnd[i].artleaf)+64*0);
         __builtin_prefetch(((char*)rnd[i].artleaf)+64*1);
         __builtin_prefetch(((char*)rnd[i].artleaf)+64*2);
         __builtin_prefetch(((char*)rnd[i].artleaf)+64*3);
         /*__builtin_prefetch(((char*)rnd[i].artleaf)+64*4);
         __builtin_prefetch(((char*)rnd[i].artleaf)+64*5);
         __builtin_prefetch(((char*)rnd[i].artleaf)+64*6);
         __builtin_prefetch(((char*)rnd[i].artleaf)+64*7);*/
         uint64_t ret;
         assert(rnd[i].artleaf->lookup(rnd[i].key,strlen(rnd[i].key),ret,rnd[i].artleaf->root));
         assert(ret==rnd[i].pos);
      }
      printf("leaflookup2 %f %f %f %f\n",(n/1000000.0)/(gettime()-start),misses.readCounter()/(float)n, branches.readCounter()/(float)n, branchMisses.readCounter()/(float)n);
   }


   {
      double start = gettime();
      map<char*, uint64_t, cmp_str> tree;
      for (uint64_t i=0;i<n;i++)
         tree[keys[i]]=i;
      printf("insertstl %f\n",(n/1000000.0)/(gettime()-start));

      PerfEvent misses(PERF_TYPE_HARDWARE,PERF_COUNT_HW_CACHE_MISSES);
      start = gettime();
      for (uint64_t i=0;i<n;i++)
         assert(tree[rnd[i].key]==rnd[i].pos);
      printf("lookupstl %f %f\n",(n/1000000.0)/(gettime()-start),misses.readCounter()/(float)n);
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
cachemisses=14.7
height=18 (avg)
height=31 (max)
space=158MB+386MB
N4: 9.6228 N16: 6.57854 N48: 1.9264 N256: 0.0749896

HAT trie:
lookup=1.5M/s
cachemisses=9.7

STL map:
lookup=0.44M/s
cachemisses=58

leaflookup 1.259591 27.127176 316.908722 17.400043
leaflookup2 2.773238 11.699756 79.182526 6.251957

lookup dist btreeART nodes:
n3:0.34 n4:0.20 n16:0.45 n256:0.008

BART 2.8 8.0 12  = 1.8
Btre 1.2 3.0 4.6 = 0.7
 */
