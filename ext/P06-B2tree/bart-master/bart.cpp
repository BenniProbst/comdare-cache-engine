#include <iostream>
#include <algorithm>
#include <atomic>
#include <cstring>
#include <string>
#include <cstddef>
#include <cstdio>
#include <csignal>
#include <x86intrin.h>
#include <utility>
#include <vector>
#include <memory>
#include "profile.hpp"
#include "utils.hpp"

using namespace std;

#define print(expr) cout << (#expr) << " " << (expr) << " "
#define println(expr) cout << (#expr) << " " << (expr) << endl
#ifdef NDEBUG
#define assert(expr) if (!(expr)) __builtin_unreachable();
#else
#define assert(expr) if (!(expr)) { fprintf(stderr, "%s:%d: %s: Assertion `%s' failed.", __FILE__, __LINE__, __PRETTY_FUNCTION__, __STRING(expr)); raise(SIGTRAP); }
#endif
#define trap raise(SIGTRAP);
#define trapexpr(expr) if (!!(expr)) raise(SIGTRAP);

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint16_t ptr;
typedef atomic<ptr> aptr;

static unsigned innerCount=0;
static unsigned leafCount=0;

struct Page;

template<class T> static void acpy(T* dst,T* src,unsigned count) { memcpy(dst,src,count*sizeof(T)); }
static void push(vector<u8>& dst,u8* src,unsigned count) { dst.resize(dst.size()+count); memcpy(dst.data()+(dst.size()-count),src,count); }
static void pop(vector<u8>& dst,unsigned count) { assert(dst.size()>=count); dst.resize(dst.size()-count); }

struct PageHeader {
   static const unsigned pageSize=32*1024;

   bool isLeaf;
   u16 spaceUsed;
   u16 offset;
   struct __attribute__ ((__packed__)) {
      u8 count;
      u8 padding;
      ptr child;
      u16 spanCount;
      u8 key[8];
      inline Page*& getPage() { return *reinterpret_cast<Page**>(key); };
   } upper;
   aptr root;
Page* previousPage = nullptr, *nextPage = nullptr;
   PageHeader(bool isLeaf_) : isLeaf(isLeaf_),spaceUsed(0),root(0) { if (isLeaf_) leafCount++; else innerCount++; upper.count=1; upper.child=0; upper.spanCount=8; upper.getPage()=nullptr; }
   ~PageHeader() {
      if (isLeaf)
         leafCount--;
      else
         innerCount--;
   }
   inline bool isInner() { return !isLeaf; }
};

struct Page : public PageHeader {
   struct NodeSpan;
   struct NodeLinear;
   struct Node256;
   struct Leaf256;

   struct  __attribute__ ((__packed__)) Node {
      static const uint8_t Node256marker=254;
      static const uint8_t Leaf256marker=255;
      uint8_t count;

      inline bool isSpan() { return count==1; }
//      inline bool isSpanLeaf() { return (count==1); }
      inline bool isLinear() { return (count>1)&&(count<17); }
      inline bool isNode2() { return count==2; }
      inline bool isNode4() { return (count==3)||(count==4); }
      inline bool isNode16() { return (count<17)&&(count>4); }
      inline bool isNode256() { return count==Node256marker; }
      inline bool isLeaf256() { return count==Leaf256marker; }

      inline NodeSpan* castSpan() { assert(isSpan()); return reinterpret_cast<NodeSpan*>(this); }
      inline NodeLinear* castLinear() { assert(isLinear()); return reinterpret_cast<NodeLinear*>(this); }
      inline Node256* castNode256() { assert(isNode256()); assert(count==Node256marker); return reinterpret_cast<Node256*>(this); }
      inline Leaf256* castLeaf256() { assert(isLeaf256()); assert(count==Leaf256marker); return reinterpret_cast<Leaf256*>(this); }
   };

   struct NodeSpan : public Node {
      //count=1
      u8 padding;
      aptr child;
      u16 spanCount;
      u8 key[];

      inline Page*& getPage() { assert(!child); assert(spanCount>=8); return *reinterpret_cast<Page**>(key+spanCount-8); }
      inline uint64_t& getInt() { assert(!child); assert(spanCount>=8); return *reinterpret_cast<uint64_t*>(key+spanCount-8); }
      inline uint64_t* getIntPtr() { assert(!child); assert(spanCount>=8); return reinterpret_cast<uint64_t*>(key+spanCount-8); }
      inline unsigned size() { return sizeof(NodeSpan)+spanCount; }
      inline bool hasChild() { return (child != 0); }
   };
   static_assert(sizeof(NodeSpan)==6,"unexpected NodeSpan size");

   struct __attribute__ ((__packed__)) NodeLinear : public Node {
      //count>1 && count<17
      u8 key[];

      inline aptr* child() { return reinterpret_cast<atomic<ptr>*>(reinterpret_cast<u8*>(key)+count); }
      inline ptr child(unsigned pos) { return child()[pos]; }
      inline unsigned size() { return sizeof(NodeLinear)+count+count*2; }
   };
   static_assert(sizeof(NodeLinear)==1,"unexpected NodeLinear size");

   struct Node256 : public Node {
      //  In Node256, each child p is
      //  - 0, if there is no greater key (!p)
      //  - the smallest key greater than this key (hasChild(p)==false)
      //  - a ptr to a child node (hasChild(p)==true)
      struct LeafValue {
         aptr p;

         inline bool hasChild() { return p&&(p<(0xFFFF-256)); }
         inline unsigned getNext() { assert(!hasChild()); return p-(0xFFFF-256); }
         inline void setNext(unsigned x) { assert(x<256); p=x+(0xFFFF-256); }
      };

      //count=Node256marker
      u16 count; // the actual count
      LeafValue child[256];

      inline unsigned minPos() { return child[0].hasChild()?0:child[0].getNext(); }
   };
   static_assert(sizeof(Node256)==1+1+2+512,"unexpected NodeLinear size");

   struct Leaf256 : public Node {
      union LeafValue {
         uint64_t v;
         Page* page;
         uint8_t k[8];

         inline bool hasChild() { return v&&(v<(~static_cast<uint64_t>(0)-256)); }
         inline unsigned getNext() { assert(!hasChild()); return v-(~static_cast<uint64_t>(0)-256); }
         inline void setNext(unsigned x) { assert(x<256); v=x+(~static_cast<uint64_t>(0)-256); }
      };

      //count=Leaf256marker
      u16 count; // the actual count
      LeafValue child[256];

      inline unsigned minPos() { return child[0].hasChild()?0:child[0].getNext(); }
   };
/*
   struct LeafHandle {
      Page* page;

   };
*/
   u8 data[pageSize-sizeof(PageHeader)];

   Page(bool isLeaf_) : PageHeader(isLeaf_) { offset=sizeof(PageHeader); }

   template<class T> inline T* cast(ptr pos) { assert(pos); assert(pos<pageSize); return reinterpret_cast<T*>(reinterpret_cast<u8*>(this)+pos); }
   ptr uncast(Node* node) { assert(node); uintptr_t p=reinterpret_cast<u8*>(node)-reinterpret_cast<u8*>(this); assert(p); assert(p<pageSize); return static_cast<ptr>(p); }

   inline unsigned freeSpace() { return pageSize-((pageSize>(32*1024))?300:0)-offset; }
   inline bool hasSpace(u16 spaceNeeded) { return freeSpace()>=spaceNeeded; }

   static Page* makeLeaf() { return new Page(true); }
   static Page* makeInner() { return new Page(false); }

   inline NodeSpan* makeSpan(u8* key,unsigned spanCount,ptr child=0) {
      unsigned size=sizeof(NodeSpan)+spanCount;
      offset+=(offset&1); // offset must be odd to make child ptr aligned
      assert(freeSpace()>=size);
      auto node=cast<NodeSpan>(offset);
      assert((reinterpret_cast<uintptr_t>(&node->child)&1)==0);
      node->count=1;
      node->child=child;
      node->spanCount=spanCount;
      acpy(node->key,key,spanCount);
      offset+=size;
      spaceUsed+=size;
      return node;
   }

   NodeLinear* makeLinear(unsigned count) {
      assert(count>1&&count<17);
      u16 size=sizeof(NodeLinear)+count+count*2;
      offset+=!(((offset&1)+(count&1))&1); // offset must be odd or even depending on if count is odd or even
      assert(freeSpace()>=size);
      auto node=cast<Node>(offset);
      node->count=count;
      assert((reinterpret_cast<uintptr_t>(node->castLinear()->child())&1)==0);
      assert(spaceUsed<=pageSize-size);
      offset+=size;
      spaceUsed+=size;
      return static_cast<NodeLinear*>(node);
   }

   template<class T> inline T* make() {
      u16 size=sizeof(T);
      assert(freeSpace()>=size);
      T* node=cast<T>(offset);
      memset(node,0,sizeof(T));
      node->count=0;
      assert(offset<=pageSize-size);
      offset+=size;
      assert(spaceUsed<=pageSize-size);
      spaceUsed+=size;
      return node;
   }

   Node256* makeNode256() {
      offset+=(offset&1); // offset must be odd to make child ptr aligned
      Node256* n=make<Node256>();
      reinterpret_cast<Node*>(n)->count=Node::Node256marker;
      assert((reinterpret_cast<uintptr_t>(&n->child[0])&1)==0);
      return n;
   }

   Leaf256* makeLeaf256() {
      while ((offset&7)!=0) offset++;
      Leaf256* n=make<Leaf256>();
      reinterpret_cast<Node*>(n)->count=Node::Leaf256marker;
      assert((reinterpret_cast<uintptr_t>(&n->child[0])&7)==0);
      return n;
   }

   template<class T> void freeNode(T* node) { spaceUsed-=node->size(); }

   static unsigned spaceForSpan(unsigned spanSize) {
      // conservative
      return sizeof(NodeSpan)+spanSize+1;
   }

   static unsigned spaceFor(unsigned nodeCount) {
      // conservative
      if (nodeCount<17) return 1+nodeCount+nodeCount*2+1;
      else return sizeof(Node256)+1;
   }

   unsigned insertSpan(NodeSpan* node,aptr* ref,u8* k,unsigned size,unsigned depth,unsigned spanBefore,Page*** out=nullptr);
   unsigned insertLinear(NodeLinear* node,aptr* ref,u8* key,unsigned size,unsigned depth,bool lastLevel,Page*** out=nullptr);
   bool insertNode(u8* key,unsigned size,unsigned maxSize,Page*** out=nullptr);
   unsigned insertNodeInternal(u8* key,unsigned size,unsigned maxSize,Page*** out=nullptr);
   vector<u8> computeSeparator();
   unsigned computeSeparatorInternal(ptr p,vector<u8>& keyStack,vector<u8>& leftKey,vector<u8>& rightKey,unsigned fromTop,float splitRatio);

   static const uint64_t noMatch=~0ull;
   Page** lookupInner(u8* key,unsigned size);
   uint64_t lookupLeaf(u8* key,unsigned size);
   uint64_t* lookupLeafPtr(u8* key,unsigned size);
   Node* lookupLeafNode(u8* key,unsigned size);

   Page** minimum(ptr p);
   void compactify();
   void copyNode(Page* targetPage,ptr curr,aptr* ref);

   pair<Page*,Page*> split(vector<u8>& sep,unsigned maxSize);
   void splitInternal(ptr p,vector<u8>& sep,vector<u8>& path,Page* left,Page* right,unsigned maxSize);

   void showPage();
   void showTree();
   void showInternal(ptr p,vector<u8>& path,vector<Page*>& pages);
};

unsigned Page::insertNodeInternal(u8* key,unsigned size,unsigned maxSize,Page*** out) {
   if (!root) {
      unsigned spaceNeeded=spaceForSpan(size);
      if (!hasSpace(spaceNeeded))
         return spaceNeeded;
      auto span=makeSpan(key,size);
      root=uncast(span);
      if (out) *out=&span->getPage();
      return 0;
   }

   unsigned depth=0;
   aptr* ref=&root;

   while (true) {
      Node* n=cast<Node>(*ref);

      if (n->isSpan()) {
         NodeSpan* node=n->castSpan();
         unsigned pos=0;
         u8* k=key+depth;
         for (pos=0;pos<node->spanCount;pos++)
            if (k[pos]!=node->key[pos])
               // Key differs from span
               return insertSpan(node,ref,k,size,depth,pos,out);
         // Span equals key
         assert(node->child); // Key already exists (or key is prefix of existing key)
         ref=&node->child;
         depth+=node->spanCount;
         continue;
      }

      u8 byte=key[depth];

      if (n->count<6) {
         NodeLinear* node=n->castLinear();
         unsigned pos;
         auto nodeChild=node->child();
         for (pos=0;pos<node->count;pos++) {
            if (node->key[pos]==byte) {
               ref=nodeChild+pos;
               goto next;
            } else if (byte<node->key[pos])
               break;
         }
         return insertLinear(node,ref,key,size,depth,false,out); // can communicate pos
      }

      if (n->isNode16()) {
         NodeLinear* node=n->castLinear();
         __m128i cmp=_mm_cmpeq_epi8(_mm_set1_epi8(byte),_mm_loadu_si128(reinterpret_cast<__m128i*>(node->key)));
         unsigned bitfield=_mm_movemask_epi8(cmp)&((1<<n->count)-1);
         if (bitfield) { ref=&node->child()[__builtin_ctz(bitfield)]; goto next; }
         return insertLinear(node,ref,key,size,depth,depth==(maxSize-1),out);
      }

      if (n->isNode256()) {
         Node256* node=n->castNode256();
         if (node->child[byte].hasChild()) { ref=&node->child[byte].p; goto next; }
         unsigned spaceNeeded=spaceForSpan(size-depth-1);
         if (!hasSpace(spaceNeeded))
            return spaceNeeded;
         auto span=makeSpan(key+depth+1,size-depth-1);
         node->child[byte].p=uncast(span);
         if (out) *out=&span->getPage();
         for (int i=static_cast<int>(byte)-1;((i>=0)&&(!node->child[i].hasChild()));i--)
            node->child[i].setNext(byte);
         node->count++;
         return 0;
      }

      {
         Leaf256* node=n->castLeaf256();
         assert(!node->child[byte].hasChild());
         assert((size-1-depth)==8);
         node->child[byte].page=*reinterpret_cast<Page**>(key+depth+1);
         for (int i=static_cast<int>(byte)-1;((i>=0)&&(!node->child[i].hasChild()));i--)
            node->child[i].setNext(byte);
         node->count++;
         return 0;
      }

   next:
      depth++;
   }
}

unsigned Page::insertSpan(NodeSpan* node,aptr* ref,u8* k,unsigned size,unsigned depth,unsigned spanBefore,Page*** out) {
   unsigned spanAfter=node->spanCount-spanBefore-1;
   unsigned spanNew=size-depth-spanBefore-1;

   /*
     The key differs from the span at byte spanBefore. We split the
     span node by shortening span1 to spanBefore bytes, and
     creating span2 for spanAfter bytes. Between the two spans we
     put a Node2. Additionally, the new Node2 may require span3 of
     spanNew bytes. Of course, all spans are only created when
     necessary.

     example: this is our original span node
               a  <- span1
               b
               c

     insert key afgh into it:
               a    <-span1
             b/ \f  <-Node2
     span2-> c   g  <-span3
     h
   */

   unsigned spaceNeeded=spaceFor(2)+spaceForSpan(spanBefore)+spaceForSpan(spanAfter)+spaceForSpan(spanNew);
   if (!hasSpace(spaceNeeded))
      return spaceNeeded;

   auto leaf=makeSpan(k+spanBefore+1,spanNew);
   if (out) *out=&leaf->getPage();
   Node* newSubTree=leaf;

   Node* oldSubTree;
   if (spanAfter||(!node->child)) // We create an empty span if there is no child
      // Add span2
      oldSubTree=makeSpan(node->key+spanBefore+1,spanAfter,node->child); else
      oldSubTree=cast<Node>(node->child);

   // Make Node2
   auto newNode=makeLinear(2);
   bool oldKeyPos=k[spanBefore]<node->key[spanBefore];
   newNode->key[!oldKeyPos]=k[spanBefore];
   newNode->child()[!oldKeyPos]=uncast(newSubTree);
   newNode->key[oldKeyPos]=node->key[spanBefore];
   newNode->child()[oldKeyPos]=uncast(oldSubTree);
   assert(newNode->child(0)&&newNode->child(1));

   // Remove or adjust span1
   *ref=spanBefore?uncast(makeSpan(node->key,spanBefore,uncast(newNode))):uncast(newNode);
   freeNode(node);

   return 0;
}

unsigned Page::insertLinear(NodeLinear* node,aptr* ref,u8* key,unsigned size,unsigned depth,bool lastLevel,Page*** out) {
   u8 byte=key[depth];
   u8* oldKey=node->key;
   aptr* oldChild=node->child();

   if (node->count<16) {
      // Create new NodeLinear
      unsigned spaceNeeded=spaceForSpan(size-depth-1)+spaceFor(node->count+1);
      if (!hasSpace(spaceNeeded))
         return spaceNeeded;
      auto newNode=makeLinear(node->count+1);
      unsigned pos; for (pos=0;(pos<node->count)&&(byte>=node->key[pos]);pos++);
      auto leaf=makeSpan(key+depth+1,size-depth-1);
      if (out) *out=&leaf->getPage();
      u8* newKey=newNode->key;
      aptr *newChild=newNode->child();
      acpy(newKey,oldKey,pos);
      acpy(newChild,oldChild,pos);
      newKey[pos]=byte;
      newChild[pos]=uncast(leaf);
      acpy(newKey+pos+1,oldKey+pos,node->count-pos);
      acpy(newNode->child()+1+pos,oldChild+pos,node->count-pos);
      freeNode(node);
      *ref=uncast(newNode);
      return 0;
   }
   for (unsigned i=0; i<16; i++)
      assert(node->child(i));

   if (lastLevel) {
      // Grow to Leaf256
      unsigned spaceNeeded=sizeof(Leaf256)+7;
      if (!hasSpace(spaceNeeded))
         return spaceNeeded;
      auto newNode=makeLeaf256();
      newNode->count=17;
      if (out) *out=&newNode->child[key[depth]].page;
      u8 prev=0;
      for (unsigned i=0;i<node->count;i++) {
         u8 k=oldKey[i];
         auto span=cast<NodeSpan>(oldChild[i]);
         freeNode(span);
         assert(span->spanCount==8);
         newNode->child[k].page=span->getPage();
         for (unsigned j=prev;j<k;j++)
            newNode->child[j].setNext(k);
         prev=k+1;
      }
      assert((size-depth-1)==8);
      u8 greatest=max(byte,oldKey[node->count-1]);
      for (unsigned i=greatest+1;i<256;i++)
         newNode->child[i].page=nullptr;
      newNode->child[key[depth]].page=*reinterpret_cast<Page**>(key+depth+1);
      for (int i=static_cast<int>(byte)-1;((i>=0)&&(!newNode->child[i].hasChild()));i--) // fix hints for new entry
         newNode->child[i].setNext(byte);
      for (unsigned i=0;i<=greatest;i++) // make sure all hints are set
         assert(newNode->child[i].page);
      freeNode(node);
      *ref=uncast(newNode);
      return 0;
   }

   // Grow to Node256
   unsigned spaceNeeded=spaceForSpan(size-depth-1)+sizeof(Node256)+1;
   if (!hasSpace(spaceNeeded))
      return spaceNeeded;
   auto newNode=makeNode256();
   newNode->count=17;
   auto leaf=makeSpan(key+depth+1,size-depth-1);
   if (out) *out=&leaf->getPage();
   u8 prev=0;
   for (unsigned i=0;i<node->count;i++) {
      u8 k=oldKey[i];
      newNode->child[k].p=oldChild[i].load();
      // Set next hints
      for (unsigned j=prev;j<k;j++)
         newNode->child[j].setNext(k);
      prev=k+1;
   }
   u8 greatest=max(byte,oldKey[node->count-1]);
   for (unsigned i=greatest+1;i<256;i++)
      newNode->child[i].p=0;
   newNode->child[byte].p=uncast(leaf);
   for (int i=static_cast<int>(byte)-1;((i>=0)&&(!newNode->child[i].hasChild()));i--) // fix hints for new entry
      newNode->child[i].setNext(byte);
   for (unsigned i=0;i<=greatest;i++) // make sure all hints are set
      assert(newNode->child[i].p);
   freeNode(node);
   *ref=uncast(newNode);
   return 0;
}


bool Page::insertNode(u8* key,unsigned size,unsigned maxSize,Page*** out) {
   unsigned spaceNeeded=insertNodeInternal(key,size,maxSize,out);
   if (spaceNeeded==0)
      return true;
   // Not enough space
   unsigned spaceAfterCompaction=pageSize-((pageSize>(32*1024))?300:0)-sizeof(PageHeader)-spaceUsed;
   if ((spaceAfterCompaction>spaceNeeded)&&((spaceAfterCompaction-freeSpace())>(pageSize/16))) {
      // Compactify will help
      compactify();
      //assert(spaceLeft()==spaceAfterCompaction);
      unsigned result=insertNodeInternal(key,size,maxSize,out);
      assert(result==0);
      return true;
   }
   return false;
}


Page** Page::minimum(ptr p) {
   while (true) {
      auto n=cast<Node>(p);
      if (n->isSpan()) {
         auto node=n->castSpan();
         if (!node->child)
            return &node->getPage();
         p=node->child;
      } else if (n->isLinear()) {
         auto node=n->castLinear();
         p=*node->child();
      } else if (n->isNode256()) {
         auto node=n->castNode256();
         p=node->child[node->minPos()].p;
      } else {
         auto node=n->castLeaf256();
         return &node->child[node->minPos()].page;
      }
   }
}

void Page::copyNode(Page* targetPage,ptr curr,aptr* ref) {
   Node* n=cast<Node>(curr);
   if (n->isSpan()) {
      auto node=n->castSpan();
      auto targetNode=targetPage->makeSpan(node->key,node->spanCount);
      while (node->child&&(cast<Node>(node->child)->isSpan())) {
         // Merge adjacent spans
         curr=node->child;
         node=cast<NodeSpan>(curr);
         acpy(targetNode->key+targetNode->spanCount,node->key,node->spanCount);
         targetPage->spaceUsed+=node->spanCount;
         targetPage->offset+=node->spanCount;
         targetNode->spanCount+=node->spanCount;
      }
      if (node->child)
         copyNode(targetPage,node->child,&targetNode->child);
      *ref=targetPage->uncast(targetNode);
      return;
   }

   if (n->count<17) {
      auto node=n->castLinear();
      auto targetNode=targetPage->makeLinear(node->count);
      acpy(targetNode->key,node->key,node->count);
      aptr* oldChild=node->child(), *newChild=targetNode->child();
      for (unsigned i=0;i<node->count;i++)
         copyNode(targetPage,oldChild[i],&newChild[i]);
      *ref=targetPage->uncast(targetNode);
      return;
   }

   if (n->isNode256()) {
      auto node=n->castNode256();
      auto targetNode=targetPage->makeNode256();
      targetNode->count=node->count;
      for (unsigned i=0;i<256;i++)
         if (node->child[i].hasChild())
            copyNode(targetPage,node->child[i].p,&targetNode->child[i].p); else
            targetNode->child[i].p=node->child[i].p.load();
      *ref=targetPage->uncast(targetNode);
      return;
   }

   {
      auto node=n->castLeaf256();
      auto targetNode=targetPage->makeLeaf256();
      targetNode->count=node->count;
      memcpy(targetNode,node,sizeof(Leaf256));
      *ref=targetPage->uncast(targetNode);
   }
}

void Page::compactify() {
   Page targetPage(isLeaf);
   targetPage.isLeaf=isLeaf;
   targetPage.upper=upper;
targetPage.previousPage = previousPage;
targetPage.nextPage = nextPage;
   copyNode(&targetPage,root,&targetPage.root);
   memcpy(this,&targetPage,sizeof(Page));
}

void Page::showPage() {
   vector<u8> path;
   vector<Page*> pages;
   showInternal(this->root,path,pages);
   cout << endl;
}

void Page::showTree() {
   vector<Page*> pages{this};
   while (!pages.empty()) {
      vector<u8> path;
      Page* p=pages.front();
      swap(pages.front(),pages.back());
      pages.pop_back();
      p->showInternal(p->root,path,pages);
      cout << endl;
   }
   cout << "---" << endl;
}

void Page::showInternal(ptr p,vector<u8>& path,vector<Page*>& pages) {
   if (p==root) {
      //compactify(); p=root;
      print(this); println(spaceUsed);
      if (!p)
         return;
   }
   Node* n=cast<Node>(p);
   if (n->isSpan()) {
      NodeSpan* node=n->castSpan();
      if (node->child) {
         for (unsigned i=0;i<node->spanCount;i++)
            path.push_back(node->key[i]);
         showInternal(node->child,path,pages);
         for (unsigned i=0;i<node->spanCount;i++)
            path.pop_back();
      } else {
         for (u8 c:path)
            cout << (int)c << ",";
         assert(node->spanCount>=8);
         for (unsigned i=0;i<node->spanCount-static_cast<unsigned>(8);i++)
            cout << (int)node->key[i] << ",";
         cout << ": ";
         if (isLeaf) {
            cout << node->getInt();
         } else {
            cout << node->getPage();
            pages.push_back(node->getPage());
         }
         cout << endl;
      }
   } else if (n->isLinear()) {
      NodeLinear* node=n->castLinear();
      aptr* nodeChild=node->child();
      for (unsigned i=0;i<node->count;i++) {
         path.push_back(node->key[i]);
         showInternal(nodeChild[i],path,pages);
         path.pop_back();
      }
   } else if (n->isNode256()) {
      Node256* node=n->castNode256();
      for (unsigned i=0;i<256;i++) {
         if (node->child[i].hasChild()) {
            path.push_back(i);
            showInternal(node->child[i].p,path,pages);
            path.pop_back();
         }
      }
   } else {
      auto node=n->castLeaf256();
      cout << "leaf count:" << node->count << " ";
      for (u8 c:path)
         cout << (int)c << ",";
      cout << endl;
   }
   if ((p==root)&&isInner()) {
      println(upper.getPage());
      pages.push_back(upper.getPage());
   }
}

vector<u8> Page::computeSeparator() {
   for (auto s : {0.4}) {
      vector<u8> keyStack;
      vector<u8> leftKey,rightKey;
      computeSeparatorInternal(root,keyStack,leftKey,rightKey,0,s);

      /*if (isLeaf) {
         unsigned common;
         for (common=0; (common<min(leftKey.size(),rightKey.size()))&&(leftKey[common]==rightKey[common]); common++);
         unsigned diff=rightKey[common]-leftKey[common];
         if (common&&(leftKey.size()>common)&&(rightKey.size()>common)&&(diff>1)) {
            leftKey[common]+=(diff/2);
            leftKey.resize(common+1);
            leftKey.push_back(0);
         }
         if (!leftKey.empty())
            return leftKey;
            }*/
      if (leftKey.size())
         return leftKey;
      if (rightKey.size())
         return rightKey;
   }
   throw;
}

unsigned Page::computeSeparatorInternal(ptr p,vector<u8>& keyStack,vector<u8>& leftKey,vector<u8>& rightKey,unsigned fromTop,float splitRatio) {
   const unsigned splitMin=pageSize*splitRatio;
   const unsigned splitMax=pageSize*(1-splitRatio);
   Node* n=cast<Node>(p);

   if (n->isSpan()) {
      NodeSpan* node=n->castSpan();
      unsigned nodeSpace=spaceForSpan(node->spanCount);
      if (!node->child) {
         assert(node->spanCount>=8);
         unsigned cnt=node->spanCount-static_cast<unsigned>(8);
         push(keyStack,node->key,cnt);
         unsigned totalCost=nodeSpace+fromTop;
         if ((totalCost>splitMin)&&leftKey.empty())
            leftKey=keyStack;
         else if ((totalCost>splitMax)&&rightKey.empty())
            rightKey=keyStack;
         pop(keyStack,cnt);
         return nodeSpace;
      }
      push(keyStack,node->key,node->spanCount);
      unsigned childSum=computeSeparatorInternal(node->child,keyStack,leftKey,rightKey,fromTop+nodeSpace,splitRatio);
      pop(keyStack,node->spanCount);
      return nodeSpace+childSum;
   }

   if (n->isLinear()) {
      NodeLinear* node=n->castLinear();
      aptr* nodeChild=node->child();
      unsigned childSum=0;
      for (unsigned i=0;i<node->count;i++) {
         keyStack.push_back(node->key[i]);
         childSum+=computeSeparatorInternal(nodeChild[i],keyStack,leftKey,rightKey,fromTop+spaceFor(i+1)+childSum,splitRatio);
         keyStack.pop_back();
      }
      return spaceFor(n->count)+childSum;
   }

   if (n->isNode256()) {
      Node256* node=n->castNode256();
      unsigned childSum=0;
      unsigned count=0;
      // XXX: if node->count is small, can use next hints
      for (unsigned i=0;i<256;i++) {
         if (node->child[i].hasChild()) {
            keyStack.push_back(i);
            childSum+=computeSeparatorInternal(node->child[i].p,keyStack,leftKey,rightKey,fromTop+spaceFor(++count)+childSum,splitRatio);
            keyStack.pop_back();
         }
      }
      return sizeof(Node256)+childSum;
   }

   {
      assert(n->isLeaf256());
      keyStack.push_back(255);
      unsigned totalCost=sizeof(Leaf256)+fromTop;
      if ((totalCost>splitMin)&&leftKey.empty())
         leftKey=keyStack;
      else if ((totalCost>splitMax)&&rightKey.empty())
         rightKey=keyStack;
      keyStack.pop_back();
      return sizeof(Leaf256);
   }
}

Page** Page::lookupInner(u8* key,unsigned size) {
   assert(isInner());
   if (!root)
      return &upper.getPage();
   Node* n=cast<Node>(root);
   unsigned depth=0;

   ptr next2=uncast(reinterpret_cast<NodeSpan*>(&upper));

   while (true) {
      assert(depth<=size);

      if (depth==size)
         return &n->castSpan()->getPage();

      if (n->isSpan()) {
         auto node=n->castSpan();
         //__builtin_prefetch(this+node->child);
         unsigned spanCount=node->spanCount-(node->child?0:8);
         unsigned repeat=min(spanCount,size-depth);
         int cmp=memcmp(key+depth,node->key,repeat);
         if (cmp<0)
            return minimum(uncast(n)); // less
         else if (cmp>0)
            return minimum(next2); // greater
         depth+=repeat;
         if (depth==size) // key exhausted
            if (!node->child)
               return minimum(uncast(n));

         // span is equal to key
         if (!node->child)
            return minimum(next2);

         n=cast<Node>(node->child);
         continue;
      }

      u8 byte=key[depth++];

      if (n->isLinear()) {
         auto node=n->castLinear();
         aptr* nodeChild=node->child();
         // Find key
         unsigned pos;
         for (pos=0;pos<n->count;pos++)
            if (byte<=node->key[pos])
               break;
         if (pos==n->count)
            // Last pos
            return minimum(next2);
         if (pos+1<n->count)
            next2=nodeChild[pos+1];
         if (byte!=node->key[pos])
            // Key not found
            return minimum(nodeChild[pos]);
         // Key found
         n=cast<Node>(nodeChild[pos]);
         continue;
      } else if (n->isNode256()) {
         auto node=n->castNode256();
         // if real child is proper ptr goto child (set next2 to following if it exists)
         // else if (no right sibling) goto min(next2)
         // else jump to right sibling (set next2 to following if it exists)
         if (!node->child[byte].p)
            // Last pos
            return minimum(next2);
         if (!node->child[byte].hasChild())
            // Key not found
            return minimum(node->child[node->child[byte].getNext()].p);
         // Key found
         if ((byte<255)&&node->child[byte+1].p)
            // Right sibling
            next2=node->child[byte+1].hasChild()?node->child[byte+1].p:node->child[node->child[byte+1].getNext()].p;
         n=cast<Node>(node->child[byte].p);
         continue;
      } else {
         auto node=n->castLeaf256();
         if (!node->child[byte].page)
            // Last pos
            return minimum(next2);
         if (!node->child[byte].hasChild())
            // Key not found
            return &node->child[node->child[byte].getNext()].page;
         return &node->child[byte].page;
      }
   }
}

uint64_t Page::lookupLeaf(u8* key,unsigned size) {
   assert(isLeaf);
   if (!root)
      return noMatch;
   Node* n=cast<Node>(root);
   unsigned depth=0;

   while (true) {
   next:
      assert(depth<=size);

      if (depth==size)
         return n->castSpan()->getInt();

      if (n->isSpan()) {
         auto node=n->castSpan();
         //__builtin_prefetch(this+node->child);
         unsigned repeat=min(static_cast<unsigned>(node->spanCount),size-depth);
         if (memcmp(key+depth,node->key,repeat)!=0)
            return noMatch;
         depth+=repeat;
         if (depth==size) {
            assert(!node->child);
            return node->getInt();
         }

         if (!node->child)
            return noMatch;

         n=cast<Node>(node->child);
         continue;
      }

      u8 byte=key[depth++];

      if (n->count<6) {
         auto node=n->castLinear();
         aptr* nodeChild=node->child();
         if (node->key[0]==byte) { n=cast<Node>(nodeChild[0]); continue; }
         if (node->key[1]==byte) { n=cast<Node>(nodeChild[1]); continue; }
         for (unsigned i=2;i<n->count;i++) {
            if (node->key[i]==byte) {
               n=cast<Node>(nodeChild[i]);
               goto next;
            }
         }
         return noMatch;
      }

      if (n->isNode16()) {
         auto node=n->castLinear();
         __m128i cmp=_mm_cmpeq_epi8(_mm_set1_epi8(byte),_mm_loadu_si128(reinterpret_cast<__m128i*>(node->key)));
         unsigned bitfield=_mm_movemask_epi8(cmp)&((1<<n->count)-1);
         if (bitfield) {
            n=cast<Node>(node->child(__builtin_ctz(bitfield)));
            goto next;
         }
         return noMatch;
      }

      if (n->isNode256()) {
         auto node=n->castNode256();
         if (node->child[byte].hasChild()) {
            n=cast<Node>(node->child[byte].p);
            goto next;
         }
         return noMatch;
      }

      {
         auto node=n->castLeaf256();
         if (node->child[byte].v) {
            assert(node->child[byte].hasChild());
            return node->child[byte].v;
         }
         return noMatch;
      }
   }
}

uint64_t* Page::lookupLeafPtr(u8* key, unsigned size) {
   assert(isLeaf);
   if (!root)
      return nullptr;
   Node* n=cast<Node>(root);
   unsigned depth=0;

   while (true) {
   next:
      assert(depth<=size);

      if (depth==size)
         return n->castSpan()->getIntPtr();

      if (n->isSpan()) {
         auto node=n->castSpan();
         //__builtin_prefetch(this+node->child);
         unsigned repeat=min(static_cast<unsigned>(node->spanCount),size-depth);
         if (memcmp(key+depth,node->key,repeat)!=0)
            return nullptr;
         depth+=repeat;
         if (depth==size) {
            assert(!node->child);
            return node->getIntPtr();
         }

         if (!node->child)
            return nullptr;

         n=cast<Node>(node->child);
         continue;
      }

      u8 byte=key[depth++];

      if (n->count<6) {
         auto node=n->castLinear();
         aptr* nodeChild=node->child();
         if (node->key[0]==byte) { n=cast<Node>(nodeChild[0]); continue; }
         if (node->key[1]==byte) { n=cast<Node>(nodeChild[1]); continue; }
         for (unsigned i=2;i<n->count;i++) {
            if (node->key[i]==byte) {
               n=cast<Node>(nodeChild[i]);
               goto next;
            }
         }
         return nullptr;
      }

      if (n->isNode16()) {
         auto node=n->castLinear();
         __m128i cmp=_mm_cmpeq_epi8(_mm_set1_epi8(byte),_mm_loadu_si128(reinterpret_cast<__m128i*>(node->key)));
         unsigned bitfield=_mm_movemask_epi8(cmp)&((1<<n->count)-1);
         if (bitfield) {
            n=cast<Node>(node->child(__builtin_ctz(bitfield)));
            goto next;
         }
         return nullptr;
      }

      if (n->isNode256()) {
         auto node=n->castNode256();
         if (node->child[byte].hasChild()) {
            n=cast<Node>(node->child[byte].p);
            goto next;
         }
         return nullptr;
      }

      {
         auto node=n->castLeaf256();
         if (node->child[byte].v) {
            assert(node->child[byte].hasChild());
            return &node->child[byte].v;
         }
         return nullptr;
      }
   }
}

pair<Page*,Page*> Page::split(vector<u8>& sep,unsigned maxSize) {
   vector<u8> path;
   path.reserve(sep.size()*2);
   Page* left=new Page(this->isLeaf);
   Page* right=new Page(this->isLeaf);
   splitInternal(this->root,sep,path,left,right,maxSize);
   left->compactify();
   right->compactify();
   right->upper=this->upper;

right->previousPage = this->previousPage;
right->nextPage = this->nextPage;

//   right->nextPage = this->next;
/*
if (left->isLeaf) {
   left->upper.getPage() = right;
}
*/
   return {left,right};
}

void Page::splitInternal(ptr p,vector<u8>& sep,vector<u8>& path,Page* left,Page* right,unsigned maxSize) {
   assert(root);
   Node* n=cast<Node>(p);
   if (n->isSpan()) {
      NodeSpan* node=n->castSpan();
      push(path,node->key,node->spanCount);
      if (node->child) {
         splitInternal(node->child,sep,path,left,right,maxSize);
      } else {

         if (left->upper.getPage()) {
            // left is finished
            right->insertNode(path.data(),path.size(),maxSize);
         } else {
            int cmp=memcmp(path.data(),sep.data(),min(path.size()-8,sep.size()));
            if (cmp<0) {
               left->insertNode(path.data(),path.size(),maxSize);
            } else {
               if (this->isLeaf) {
                  ((cmp==0)/*&&(path.size()<=sep.size())*/?left:right)->insertNode(path.data(),path.size(),maxSize);
                  left->upper.getPage()=reinterpret_cast<Page*>(1);
//                  left->upper.getPage() = right;
               } else {
                  // don't insert first key that is >= than separator
                  left->upper.getPage()=node->getPage();
               }
            }
         }

      }
      pop(path,node->spanCount);
   } else if (n->isLinear()) {
      NodeLinear* node=n->castLinear();
      aptr* nodeChild=node->child();
      for (unsigned i=0;i<node->count;i++) {
         path.push_back(node->key[i]);
         splitInternal(nodeChild[i],sep,path,left,right,maxSize);
         path.pop_back();
      }
   } else if (n->isNode256()) {
      Node256* node=n->castNode256();
      for (unsigned i=0;i<256;i++) {
         if (node->child[i].hasChild()) {
            path.push_back(i);
            splitInternal(node->child[i].p,sep,path,left,right,maxSize);
            path.pop_back();
         }
      }
   } else {
      Leaf256* node=n->castLeaf256();
      // XXX: move the entire node! (one memcpy)
      for (unsigned i=0;i<256;i++) {
         if (node->child[i].page) {
            path.push_back(i);
            for (unsigned j=0; j<8; j++)
               path.push_back(node->child[i].k[j]);
            //(left->upper?right:left)->insertNode(path.data(),path.size());

            if (left->upper.getPage()) {
               // left is finished
               right->insertNode(path.data(),path.size(),maxSize);
            } else {
               int cmp=memcmp(path.data(),sep.data(),min(path.size()-8,sep.size()));
               if (cmp<0) {
                  left->insertNode(path.data(),path.size(),maxSize);
               } else {
                  if (this->isLeaf) {
                     ((cmp==0)/*&&(path.size()<=sep.size())*/?left:right)->insertNode(path.data(),path.size(),maxSize);
                     left->upper.getPage()=reinterpret_cast<Page*>(1);
//                     left->upper.getPage() = right;
                  } else {
                     // don't insert first key that is >= than separator
                     left->upper.getPage()=node->child[i].page;
                  }
               }
            }

            for (unsigned j=0; j<8; j++)
               path.pop_back();
            path.pop_back();
         }
      }
   }
}

namespace Range {

struct NodeIterationState {
   Page::Node* node;
   unsigned childIdx;
   unsigned visitedChildCount;
   NodeIterationState(Page::Node* node) : node(node), childIdx(0u), visitedChildCount(0u)
   {}
   NodeIterationState(Page::Node* node, unsigned childIdx) : node(node), childIdx(childIdx), visitedChildCount(0u)
   {}
   NodeIterationState(Page::Node* node, unsigned childIdx, unsigned visitedChildCount) : node(node), childIdx(childIdx), visitedChildCount(visitedChildCount)
   {}
};

struct PageIterationState {
   Page* leaf;
//   std::vector<std::unique_ptr<NodeIterationState>> nodeStateStack;
   std::vector<NodeIterationState> nodeStateStack;
};

static inline bool nodeExhausted(const NodeIterationState& state) {
   if (state.node->isSpan()) {
      return (state.childIdx > 0);
   } else if (state.node->isNode256()) {
      Page::Node256* node256 = state.node->castNode256();
      assert(state.visitedChildCount < 256);
      return (state.visitedChildCount >= node256->count || state.childIdx > 255);
   } else if (state.node->isLeaf256()) {
      Page::Leaf256* leaf = state.node->castLeaf256();
      assert(state.visitedChildCount < 256);
      return (state.visitedChildCount >= leaf->count || state.childIdx > 255);
   } else {
      return (state.childIdx >= state.node->count);
   }
}

static inline bool isLeafNode(Page::Node* node) {
   return (node->isLeaf256() || (node->isSpan() && !node->castSpan()->hasChild()));
}

static inline void fetchNextValue(NodeIterationState& nodeState, uint64_t** resultPtr) {
   assert(!nodeExhausted(nodeState));

   auto* node = nodeState.node;
   assert(isLeafNode(node));
   if (node->isSpan()) {
      Page::NodeSpan* nodeSpan = node->castSpan();
      assert(!nodeSpan->hasChild());
      *resultPtr = nodeSpan->getIntPtr();
   } else if (node->isLeaf256()) {
      Page::Leaf256* leaf = node->castLeaf256();

      if (!leaf->child[nodeState.childIdx].hasChild()) {
         unsigned nextIdx = leaf->child[nodeState.childIdx].getNext();
         nodeState.childIdx = std::min(nextIdx, 255u);
         return fetchNextValue(nodeState, resultPtr); // delegate
      }

      *resultPtr = &leaf->child[nodeState.childIdx].v;
      nodeState.visitedChildCount += 1;
   } else {
      assert(false);
   }
   nodeState.childIdx += 1;
}

static bool advancePage(PageIterationState& state) {
//   Page* next = state.leaf->upper.getPage();
   Page* next = state.leaf->nextPage;
   if (next == nullptr) {
//      cout << "reached last leaf page" << endl;
      return false;
   }
   state.leaf = next;
   state.nodeStateStack.clear();
   return true;
}

static void initialDescent(PageIterationState& state, u8* key, unsigned size) {
   assert(state.leaf->isLeaf);
   if (!state.leaf->root) { return; }

   Page::Node* n = state.leaf->cast<Page::Node>(state.leaf->root);
   unsigned depth = 0;

   while (true) {
   next:
      assert(depth<=size);

      if (n->isSpan()) {
         auto nodeSpan = n->castSpan();
         unsigned repeat = min(static_cast<unsigned>(nodeSpan->spanCount), size-depth);
         if (memcmp(key+depth, nodeSpan->key, repeat) != 0) {
            throw std::runtime_error("not implemented");
         }

         depth += repeat;
         if (depth == size) {
            assert(!nodeSpan->hasChild());
            state.nodeStateStack.push_back(NodeIterationState(nodeSpan));
            return;
         }

         if (!nodeSpan->child) {
            throw std::runtime_error("not implemented");
         }

         state.nodeStateStack.push_back(NodeIterationState(nodeSpan, 1u));
         n = state.leaf->cast<Page::Node>(nodeSpan->child);
      } else if (n->count < 6) {
         // NodeLinear
         u8 byte=key[depth++];
         auto nodeLinear = n->castLinear();
         aptr* nodeChild = nodeLinear->child();
         for (unsigned i = 0; i < n->count; i++) {
            if (nodeLinear->key[i] == byte) {
               state.nodeStateStack.push_back(NodeIterationState(n, i + 1));
               n = state.leaf->cast<Page::Node>(nodeChild[i]);
               goto next;
            }
         }

         throw std::runtime_error("not implemented");
      } else if (n->isNode16()) {
         // NodeLinear
         u8 byte=key[depth++];
         auto nodeLinear = n->castLinear();
         __m128i cmp = _mm_cmpeq_epi8(_mm_set1_epi8(byte), _mm_loadu_si128(reinterpret_cast<__m128i*>(nodeLinear->key)));
         unsigned bitfield = _mm_movemask_epi8(cmp) & ((1 << n->count) - 1);
         if (bitfield) {
            unsigned childIdx = __builtin_ctz(bitfield);
            unsigned nextIdx = childIdx + 1;
            state.nodeStateStack.push_back(NodeIterationState(n, nextIdx));
            n = state.leaf->cast<Page::Node>(nodeLinear->child(childIdx));
            goto next;
         }

         throw std::runtime_error("not implemented");
      } else if (n->isNode256()) {
         u8 byte=key[depth++];
         auto node256 = n->castNode256();
         if (node256->child[byte].hasChild()) {
            unsigned nextIdx = byte + 1; // may be largen than 255
            state.nodeStateStack.push_back(NodeIterationState(n, nextIdx));
            n = state.leaf->cast<Page::Node>(node256->child[byte].p);
            goto next;
         }

         throw std::runtime_error("not implemented");
      } else {
         // Leaf256
         u8 byte=key[depth++];
         auto leaf = n->castLeaf256();
         if (leaf->child[byte].v) {
            assert(leaf->child[byte].hasChild());
            unsigned nextIdx = byte + 1; // may be largen than 255
            state.nodeStateStack.push_back(NodeIterationState(n, nextIdx));
            return;
         }

         throw std::runtime_error("not implemented");
      }
   }
}

static bool traverseToLeafNode(PageIterationState& state) {
   auto& nodeState = state.nodeStateStack.back();
   if (nodeExhausted(nodeState)) {
      return false;
   }

   Page::Node* node = nodeState.node;
   Page::Node* child = nullptr;
   if (node->isSpan()) {
      Page::NodeSpan* nodeSpan = node->castSpan();
//printf("%.*s", nodeSpan->spanCount, nodeSpan->key);

      if (!nodeSpan->hasChild()) {
         // new leaf node
         return true;
      }

      ptr p = nodeSpan->child;
      child = state.leaf->cast<Page::Node>(p);
   } else if (node->isLinear()) {
      Page::NodeLinear* nodeLinear = node->castLinear();
//printf("%c", nodeLinear->key[nodeState.childIdx]);
      ptr p = nodeLinear->child(nodeState.childIdx);
      child = state.leaf->cast<Page::Node>(p);
   } else if (node->isNode256()) {
      Page::Node256* node256 = node->castNode256();

      if (!node256->child[nodeState.childIdx].hasChild()) {
         unsigned nextIdx = node256->child[nodeState.childIdx].getNext();
         assert(nextIdx > nodeState.childIdx);
//cout << "nextIdx: " << nextIdx << endl;
         nodeState.childIdx = nextIdx;
         return traverseToLeafNode(state); // delegate
      }

//printf("%c", static_cast<char>(nodeState.childIdx));
      ptr p = node256->child[nodeState.childIdx].p;
      child = state.leaf->cast<Page::Node>(p);
      nodeState.visitedChildCount += 1;
   } else {
      assert(node->isLeaf256());
      // new leaf node
      return true;
   }

   nodeState.childIdx += 1; // prepare next iteration
   state.nodeStateStack.push_back(NodeIterationState(child));
   return traverseToLeafNode(state);
}

static inline bool hasUnvisitedLeaf(const PageIterationState& state) {
    if (state.nodeStateStack.empty()) { return false; }
    const auto& last = state.nodeStateStack.back();
    return (isLeafNode(last.node) && last.childIdx == 0);
}

static bool advanceWithinPage(PageIterationState& state, uint64_t** resultPtr) {
   if (state.nodeStateStack.empty()) {
      // new page
      if (state.leaf->root == 0) { return false; } // empty page
      Page::Node* node = state.leaf->cast<Page::Node>(state.leaf->root);
      state.nodeStateStack.push_back(NodeIterationState(node));
   }

   bool unvisitedLeafNode = hasUnvisitedLeaf(state);
   do {
      if (!isLeafNode(state.nodeStateStack.back().node)) {
//         cout << "navigate to new leaf" << endl;
         unvisitedLeafNode = traverseToLeafNode(state);
//         cout << endl;
      }
      if (nodeExhausted(state.nodeStateStack.back())) {
//         cout << "current node exhausted" << endl;
         state.nodeStateStack.pop_back();
      }
   } while (!state.nodeStateStack.empty() && !unvisitedLeafNode);

   if (state.nodeStateStack.empty()) { return false; } // page exhausted

   auto& nodeState = state.nodeStateStack.back();
   assert(isLeafNode(nodeState.node));
   fetchNextValue(nodeState, resultPtr);

   return true;
}

} // namespace Range

struct BART {
   Page* root;
   unsigned maxSize;

   BART(unsigned _maxSize=~static_cast<unsigned>(0)) : root(Page::makeLeaf()),maxSize(_maxSize) {}

   void insert(u8* key,unsigned size);

   unsigned levels() {
      Page* p=root;
      unsigned count=1;
      while (p->isInner()) {
         p=p->upper.getPage();
         count++;
      }
      return count;
   }

   uint64_t __attribute__ ((noinline)) lookup(u8* key,unsigned size) {
      Page* p=root;
      while (p->isInner())
         p=*(p->lookupInner(key,size));
      return p->lookupLeaf(key,size);
   }

   struct RangeIterator {
      Range::PageIterationState pageIterationState;
      uint64_t* upperLimitPtr;
      uint64_t value;
      uint64_t* valuePtr;
      bool done = false;
      bool next();
   };

   RangeIterator __attribute__ ((noinline)) createRangeIterator(u8* lower, unsigned l1, u8* upper, unsigned l2) {
      Page* leftPage = root;
      while (leftPage->isInner()) {
         leftPage = *(leftPage->lookupInner(lower, l1));
      }
      Page* rightPage = root;
      while (rightPage->isInner()) {
         rightPage = *(rightPage->lookupInner(upper, l1));
      }
      uint64_t* upperLimitPtr = rightPage->lookupLeafPtr(upper, l2);
//      printf("upperLimitPtr: %p\n", upperLimitPtr);

      RangeIterator it;
      it.pageIterationState.nodeStateStack.reserve(64);
      it.pageIterationState.leaf = leftPage;
      it.upperLimitPtr = upperLimitPtr;
      Range::initialDescent(it.pageIterationState, lower, l1);
      return it;
   }
};

bool BART::RangeIterator::next() {
   using namespace Range;

   if (done) { return false; }

   bool pageExhausted = !advanceWithinPage(pageIterationState, &valuePtr);
   if (pageExhausted) {
//      cout << "page exhausted; load new page" << endl;
      done = !advancePage(pageIterationState);
      return next();
   }

   value = *valuePtr;
   done |= (valuePtr == upperLimitPtr);

   return true;
}

void BART::insert(u8* key,unsigned size) {
   // remember inner nodes
   Page** current=&root;
   vector<Page**> innerNodes;
   while (!(*current)->isLeaf) {
      innerNodes.push_back(current);
      current=(*current)->lookupInner(key,size-8);
   }
   assert((*current)->isLeaf);
   if ((*current)->insertNode(key,size,maxSize))
      return;

   Page** out=nullptr;
   Page** previous=innerNodes.size()?innerNodes.back():nullptr;
   vector<u8> sep;
   Page* oldPage=nullptr;
   while (true) {
      if (!previous) {
         // Make new root
         Page* newInner=Page::makeInner();
         newInner->upper.getPage()=*current;
         *current=newInner;
         previous=current;
         current=&newInner->upper.getPage();
      }
      sep=(*current)->computeSeparator(); sep.resize(sep.size()+8);
      oldPage=*current;
      if ((*previous)->insertNode(sep.data(),sep.size(),maxSize,&out))
         break;
      current=previous;
      if (innerNodes.size())
         innerNodes.pop_back();
      previous=innerNodes.size()?innerNodes.back():nullptr;
   }
   assert(out);

   // Split
   sep.resize(sep.size()-8);
   pair<Page*,Page*> pr=oldPage->split(sep,maxSize);
   *out=pr.first;
   memcpy(oldPage,pr.second,Page::pageSize);
   delete pr.second;

   if (pr.first->isLeaf) {
      pr.first->previousPage = oldPage->previousPage;
      pr.first->nextPage = oldPage;
      if (oldPage->previousPage) {
         oldPage->previousPage->nextPage = pr.first;
      }
      oldPage->previousPage = pr.first;
   }

   // Restart
   insert(key,size);
}

int main(int /*argc*/,char** argv) {
#if 0
   if (getenv("INT")) {
      unsigned nn=atof(argv[1]);
      vector<uint32_t> v;
      v.reserve(nn);
      for (unsigned i=0; i<nn; i++)
         v.push_back(i); //|(random()<<24)
      random_shuffle(v.begin(),v.end());
      BART tree(4);
      timeAndProfile("BART int insert", nn, [&]() {
            for (uint64_t i=0;i<nn;i++) {
               uint8_t key[12]; *reinterpret_cast<uint32_t*>(key)=__builtin_bswap32(v[i]); *reinterpret_cast<uint64_t*>(key+4)=i+1;
               tree.insert(key,12);
            }
         }, true);
      //{streambuf *old = cout.rdbuf(0); tree.root->showTree(); cout.rdbuf(old);}
      timeAndProfile("BART int lookup", nn, [&]() {
            for (uint64_t j=0;j<nn;j++) {
               uint8_t key[4]; *reinterpret_cast<uint32_t*>(key)=__builtin_bswap32(v[j]);
               uint64_t res=tree.lookup(key,4);
               if (res!=j+1)
                  throw;
            }
         });
      return 0;
   }
#endif

   FILE* file=fopen(argv[1],"r");
   if (!file) {
      cerr << "error opening file" << endl;
      throw;
   }
   vector<char*> keys;
   char* buffvar=nullptr;
   size_t leng=0;
   unsigned n=0;
   while (getline(&buffvar, &leng, file)!=-1) {
      n++;
      for (unsigned i=0; i<strlen(buffvar); i++)
         if (buffvar[i]=='\n')
            buffvar[i]='x';
      keys.push_back(buffvar);
      buffvar=nullptr;
   }

   for (unsigned i=0; i<n; i++) {
      unsigned len=strlen(keys[i])+1;
      char* key=new char[len+8];
      memcpy(key,keys[i],len);
      *reinterpret_cast<uint64_t*>(key+len)=i;
      //free(keys[i]);
      keys[i]=key;
   }

   BART tree;

   size_t repetitions = 1;
   char* repetitionsEnv = getenv("REPETITIONS");
   if (repetitionsEnv) {
      repetitions = std::atoi(repetitionsEnv);
   }

   string dataset("'");
   dataset += getFileName(argv[1]);
   dataset += "'";

   PerfEvents e;
   e.timeAndProfile("insert", n, [&]() {
         for (uint64_t i=0;i<n;i++) {
            u8* kk=reinterpret_cast<u8*>(keys[i]);
            unsigned len=strlen(keys[i])+9;
            tree.insert(kk,len);

            /*for (uint64_t j=0;j<=i;j++) {
               u8* kkk=reinterpret_cast<u8*>(keys[j]);
               unsigned len2=strlen(keys[j])+1;
               uint64_t res=tree.lookup(kkk,len2);
               if (res!=j)
                  trap;
                  }*/
         }
      }, 1, {{"approach", "BART"}, {"dataset", dataset}});
//   cout << "levels:" << tree.levels() << " leaf:" << leafCount << " inner:" << innerCount << endl;

   //{streambuf *old = cout.rdbuf(0); tree.root->showTree(); cout.rdbuf(old);}

   e.timeAndProfile("lookup", n, [&]() {
         for (uint64_t j=0;j<n;j++) {
            u8* kkk=reinterpret_cast<u8*>(keys[j]);
            unsigned len2=strlen(keys[j])+1;
            uint64_t res=tree.lookup(kkk,len2);
            if (res!=j)
               throw;
         }
      }, repetitions, {{"approach", "BART"}, {"dataset", dataset}});


   // range benchmark:
   const unsigned intervalCount = 8;
   std::vector<char*> sortedKeys = keys;
   std::sort(sortedKeys.begin(), sortedKeys.end(), [](const char *c1, const char *c2) {
      return strcmp(c1, c2) < 0;
   });


   auto rangeFn = [&]() {
      size_t keyIdx = 0;
      const unsigned intervalSize = keys.size()/intervalCount;
      for (size_t i = 0; i < intervalCount; ++i) {
         auto lowerIdx = i*intervalSize;
         auto upperIdx = (i == intervalCount - 1) ? keys.size() - 1 : (i + 1)*intervalSize - 1;
         char* lower = sortedKeys[lowerIdx];
         char* upper = sortedKeys[upperIdx];
         unsigned len1 = strlen(lower) + 1;
         unsigned len2 = strlen(upper) + 1;
//         cout << "lower: " << lower << endl;
//         cout << "upper: " << upper << endl;
         auto it = tree.createRangeIterator(reinterpret_cast<u8*>(lower), strlen(lower), reinterpret_cast<u8*>(upper), strlen(upper));

         while (it.next()) {
            char* key = sortedKeys[keyIdx];
//            std::cout << "range lookup " << keyIdx << " for: " << key << endl;
            unsigned len = strlen(key) + 1;
            uint64_t expected = *reinterpret_cast<uint64_t*>(key + len);
            assert(it.value == expected);
            keyIdx += 1;
         }
      }
   };
   e.timeAndProfile("rangeLookup", n, rangeFn, repetitions, {{"approach", "BART"}, {"dataset", dataset}});

   return 0;
}
