#include <iostream>
#include <algorithm>
#include <vector>
#include <map>
#include <cassert>
#include "profile.cpp"
#include "cpp-btree-1.0.1/btree_map.h"

using namespace std;

enum class PageType : uint8_t { BTreeInner=1, BTreeLeaf=2 };

static const uint64_t pageSize=4*1024;

struct NodeBase {
   PageType type;
   uint16_t count;
};

struct BTreeLeafBase : public NodeBase {
   static const PageType typeMarker=PageType::BTreeLeaf;
};

template<class Key,class Payload>
struct BTreeLeaf : public BTreeLeafBase {
   struct Entry {
      Key k;
      Payload p;
   };

   static const uint64_t maxEntries=(pageSize-sizeof(NodeBase))/(sizeof(Key)+sizeof(Payload));

   Key keys[maxEntries];
   Payload payloads[maxEntries];

   BTreeLeaf() {
      count=0;
      type=typeMarker;
   }

   unsigned lowerBound(Key k) {
      unsigned lower=0;
      unsigned upper=count;
      do {
         unsigned mid=((upper-lower)/2)+lower;
         if (k<keys[mid]) {
            upper=mid;
         } else if (k>keys[mid]) {
            lower=mid+1;
         } else {
            return mid;
         }
      } while (lower<upper);
      return lower;
   }

   unsigned lowerBoundBF(Key k) {
      auto base=keys;
      unsigned n=count;
      while (n>1) {
         const unsigned half=n/2;
         //__builtin_prefetch(base+half); __builtin_prefetch(base+half+half/2);
         base=(base[half]<k)?(base+half):base;
         n-=half;
      }
      return (*base<k)+base-keys;
   }

   void insert(Key k,Payload p) {
      if (count) {
         unsigned pos=lowerBound(k);
         memmove(keys+pos+1,keys+pos,sizeof(Key)*(count-pos));
         memmove(payloads+pos+1,payloads+pos,sizeof(Payload)*(count-pos));
         keys[pos]=k;
         payloads[pos]=p;
      } else {
         keys[0]=k;
         payloads[0]=p;
      }
      count++;
   }

   BTreeLeaf* split(Key& sep) {
      BTreeLeaf* newLeaf=new BTreeLeaf();
      newLeaf->count=count-(count/2);
      count=count-newLeaf->count;
      memcpy(newLeaf->keys,keys+count,sizeof(Key)*newLeaf->count);
      memcpy(newLeaf->payloads,payloads+count,sizeof(Payload)*newLeaf->count);
      sep=keys[count-1];
      return newLeaf;
   }
};

struct BTreeInnerBase : public NodeBase {
   static const PageType typeMarker=PageType::BTreeInner;
};

template<class Key>
struct BTreeInner : public BTreeInnerBase {
   static const uint64_t maxEntries=(pageSize-sizeof(NodeBase))/(sizeof(Key)+sizeof(NodeBase*));
   NodeBase* children[maxEntries];
   Key keys[maxEntries];

   BTreeInner() {
      count=0;
      type=typeMarker;
   }

   unsigned lowerBoundBF(Key k) {
      auto base=keys;
      unsigned n=count;
      while (n>1) {
         const unsigned half=n/2;
         //__builtin_prefetch(base+half); __builtin_prefetch(base+half+half/2);
         base=(base[half]<k)?(base+half):base;
         n-=half;
      }
      return (*base<k)+base-keys;
   }

   unsigned lowerBound(Key k) {
      unsigned lower=0;
      unsigned upper=count;
      do {
         unsigned mid=((upper-lower)/2)+lower;
         if (k<keys[mid]) {
            upper=mid;
         } else if (k>keys[mid]) {
            lower=mid+1;
         } else {
            return mid;
         }
      } while (lower<upper);
      return lower;
   }

   BTreeInner* split(Key& sep) {
      BTreeInner* newInner=new BTreeInner();
      newInner->count=count-(count/2);
      count=count-newInner->count-1;
      sep=keys[count];
      memcpy(newInner->keys,keys+count+1,sizeof(Key)*(newInner->count+1));
      memcpy(newInner->children,children+count+1,sizeof(NodeBase*)*(newInner->count+1));
      return newInner;
   }

   void insert(Key k,NodeBase* child) {
      unsigned pos=lowerBound(k);
      memmove(keys+pos+1,keys+pos,sizeof(Key)*(count-pos+1));
      memmove(children+pos+1,children+pos,sizeof(NodeBase*)*(count-pos+1));
      keys[pos]=k;
      children[pos]=child;
      std::swap(children[pos],children[pos+1]);
      count++;
   }

};


template<class Key,class Value>
struct BTree {
   NodeBase* root;

   BTree() {
      root=new BTreeLeaf<Key,Value>();
   }

   void makeRoot(Key k,NodeBase* leftChild,NodeBase* rightChild) {
      auto inner=new BTreeInner<Key>();
      inner->count=1;
      inner->keys[0]=k;
      inner->children[0]=leftChild;
      inner->children[1]=rightChild;
      root=inner;
   }

   void insert(Key k,Value v) {
      NodeBase* node=root;
      BTreeInner<Key>* parent=nullptr;
      while (node->type==PageType::BTreeInner) {
         auto inner=static_cast<BTreeInner<Key>*>(node);
         if (inner->count==inner->maxEntries-1) {
            // Split inner eagerly
            Key sep;
            BTreeInner<Key>* newInner=inner->split(sep);
            if (parent)
               parent->insert(sep,newInner); else
               makeRoot(sep,inner,newInner);
            if (k>=sep)
               inner=newInner;
         }
         parent=inner;
         unsigned pos=inner->lowerBound(k);
         node=inner->children[pos];
         assert(node);
      }
      BTreeLeaf<Key,Value>* leaf=static_cast<BTreeLeaf<Key,Value>*>(node);
      if (leaf->count==leaf->maxEntries) {
         // Leaf is full, split it
         Key sep;
         BTreeLeaf<Key,Value>* newLeaf=leaf->split(sep);
         if (parent)
            parent->insert(sep,newLeaf); else
            makeRoot(sep,leaf,newLeaf);
         if (k>=sep)
            leaf=newLeaf;
      }
      leaf->insert(k,v);
   }

   Value lookup(Key k) {
      NodeBase* node=root;
      while (node->type==PageType::BTreeInner) {
         BTreeInner<Key>* inner=static_cast<BTreeInner<Key>*>(node);
         unsigned pos=inner->lowerBound(k);
         node=inner->children[pos];
      }

      BTreeLeaf<Key,Value>* leaf=static_cast<BTreeLeaf<Key,Value>*>(node);
      unsigned pos=leaf->lowerBound(k);
      return leaf->payloads[pos];
   }
};


int main(int argc,char** argv) {
   uint64_t n=atof(argv[1]);

   vector<uint32_t> v(n);
   for (uint64_t i=0; i<n; i++)
      v[i]=i;
   random_shuffle(v.begin(),v.end());

   {
      BTree<uint32_t,uint64_t> tree;
      timeAndProfile("BTree insert", n, [&]() {
            for (uint64_t i=0; i<n; i++)
               tree.insert(v[i],i);
         });

      timeAndProfile("BTree lookup", n, [&]() {
            for (uint64_t i=0; i<n; i++)
               if (tree.lookup(v[i])!=i)
                  throw;
         });

      uint32_t k=v[0];
      timeAndProfile("BTree dep", n, [&]() {
            for (uint64_t i=0; i<n; i++)
               k=tree.lookup(k);
         });
   }

   btree::btree_map<uint32_t,uint64_t> gtree;
   timeAndProfile("googleBTree insert", n, [&]() {
         for (uint64_t i=0; i<n; i++)
            gtree.insert({v[i],i});
      });

   {
      timeAndProfile("googleBTree lookup", n, [&]() {
            for (uint64_t i=0; i<n; i++)
               if ((*gtree.find(v[i])).second!=i)
                  throw;
         });

      uint32_t k=v[0];
      timeAndProfile("googleBTree dep", n, [&]() {
            for (uint64_t i=0; i<n; i++)
               k=(*gtree.find(k)).second;
         });
   }

   map<uint32_t,uint64_t> stree;
   timeAndProfile("stl insert", n, [&]() {
         for (uint64_t i=0; i<n; i++)
            stree.insert({v[i],i});
      });
   {
      timeAndProfile("stl lookup", n, [&]() {
         for (uint64_t i=0; i<n; i++)
            if ((*stree.find(v[i])).second!=i)
               throw;
         });
      uint32_t k=v[0];
      timeAndProfile("stl dep", n, [&]() {
            for (uint64_t i=0; i<n; i++)
               k=(*stree.find(k)).second;
         });
   }

   return 0;
}
