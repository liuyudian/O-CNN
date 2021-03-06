#include "octree_parser.h"
#include "octree_nn.h"
#include "logs.h"

#include <cstring>

void OctreeParser::set_cpu(const void* ptr) {
  const_ptr_ = true;
  h_metadata_ = reinterpret_cast<char*>(const_cast<void*>(ptr));
  info_ = reinterpret_cast<OctreeInfo*>(h_metadata_);
}

void OctreeParser::set_cpu(void* ptr, OctreeInfo* octinfo) {
  const_ptr_ = false;
  h_metadata_ = reinterpret_cast<char*>(ptr);
  info_ = reinterpret_cast<OctreeInfo*>(ptr);
  if (octinfo != nullptr) { // update the OctreeInfo with octinfo
    memcpy(info_, octinfo, sizeof(OctreeInfo));
  }
}

OctreeParser::NodeType OctreeParser::node_type(const int t) const {
  NodeType ntype = kInternelNode;
  if (t == -1) ntype = kLeaf;
  if (t == -2) ntype = kNonEmptyLeaf;
  return ntype;
}

const char* OctreeParser::ptr_cpu(const PropType ptype, const int depth) const {
  CHECK(h_metadata_ != nullptr);
  const char* p = nullptr;
  int dis = info_->ptr_dis(ptype, depth);
  if (-1 != dis) {
    p = h_metadata_ + dis;
  }
  return p;
}

const uint32* OctreeParser::key_cpu(const int depth) const {
  return reinterpret_cast<const uint32*>(ptr_cpu(OctreeInfo::kKey, depth));
}

const int* OctreeParser::children_cpu(const int depth) const {
  return reinterpret_cast<const int*>(ptr_cpu(OctreeInfo::kChild, depth));
}

const int* OctreeParser::neighbor_cpu(const int depth) const {
  return reinterpret_cast<const int*>(ptr_cpu(OctreeInfo::kNeigh, depth));
}

const float* OctreeParser::feature_cpu(const int depth) const {
  return reinterpret_cast<const float*>(ptr_cpu(OctreeInfo::kFeature, depth));
}

const float* OctreeParser::label_cpu(const int depth) const {
  return reinterpret_cast<const float*>(ptr_cpu(OctreeInfo::kLabel, depth));
}

const float* OctreeParser::split_cpu(const int depth) const {
  return reinterpret_cast<const float*>(ptr_cpu(OctreeInfo::kSplit, depth));
}

char* OctreeParser::mutable_ptr_cpu(const OctreeInfo::PropType ptype, const int depth) {
  CHECK(const_ptr_ == false);
  return const_cast<char*>(ptr_cpu(ptype, depth));
}

uint32* OctreeParser::mutable_key_cpu(const int depth) {
  return reinterpret_cast<uint32*>(mutable_ptr_cpu(OctreeInfo::kKey, depth));
}

int* OctreeParser::mutable_children_cpu(const int depth) {
  return reinterpret_cast<int*>(mutable_ptr_cpu(OctreeInfo::kChild, depth));
}

int* OctreeParser::mutable_neighbor_cpu(const int depth) {
  return reinterpret_cast<int*>(mutable_ptr_cpu(OctreeInfo::kNeigh, depth));
}

float* OctreeParser::mutable_feature_cpu(const int depth) {
  return reinterpret_cast<float*>(mutable_ptr_cpu(OctreeInfo::kFeature, depth));
}

float* OctreeParser::mutable_label_cpu(const int depth) {
  return reinterpret_cast<float*>(mutable_ptr_cpu(OctreeInfo::kLabel, depth));
}

float* OctreeParser::mutable_split_cpu(const int depth) {
  return reinterpret_cast<float*>(mutable_ptr_cpu(OctreeInfo::kSplit, depth));
}


//////////////////////////////////////
void OctreeParser::node_pos(float* xyz, int id, int depth, float* xyz_base) const {
  const uint32* keyi = key_cpu(depth) + id;
  key2xyz(xyz, *keyi, depth);

  if (xyz_base != nullptr) {
    for (int c = 0; c < 3; ++c) {
      xyz_base[c] = xyz[c];
    }
  }

  for (int c = 0; c < 3; ++c) {
    xyz[c] += 0.5f;
  }
  if (info_->has_displace()) {
    const float kDis = 0.8660254f; // = sqrt(3.0f) / 2.0f
    float dis = node_dis(id, depth) * kDis; // !!! Note kDis
    if (dis == 0) return;
    float n[3] = { 0 };
    node_normal(n, id, depth);
    for (int c = 0; c < 3; ++c) {
      xyz[c] += dis * n[c];
    }
  }
}

void OctreeParser::node_normal(float* n, int id, int depth) const {
  int num = info_->node_num(depth);
  const float* feature_d = feature_cpu(depth);
  int loc = info_->locations(OctreeInfo::kFeature);
  int ch = info_->channel(OctreeInfo::kFeature);
  if ((loc == -1 || loc == depth) && ch >= 3) {
    for (int c = 0; c < 3; ++c) { n[c] = feature_d[c * num + id]; }
  } else {
    for (int c = 0; c < 3; ++c) { n[c] = 0; }
  }
}

float OctreeParser::node_dis(int id, int depth) const {
  int num = info_->node_num(depth);
  const float* feature_d = feature_cpu(depth);
  int loc = info_->locations(OctreeInfo::kFeature);
  int ch = info_->channel(OctreeInfo::kFeature);
  if ((loc == -1 || loc == depth) && ch >= 4) {
    return feature_d[3 * num + id];
  } else {
    return 0;
  }
}

template<typename Dtype>
void OctreeParser::key2xyz(Dtype* xyz, const uint32& key, const int depth) const {
  if (info_->is_key2xyz()) {
    //!!! Caveat: the octree depth should be less than 8
    const unsigned char* pt = reinterpret_cast<const unsigned char*>(&key);
    for (int c = 0; c < 3; ++c) {
      xyz[c] = static_cast<Dtype>(pt[c]);
    }
  }
  else {
    uint32 pt[3];
    compute_pt(pt, key, depth);
    for (int c = 0; c < 3; ++c) {
      xyz[c] = static_cast<Dtype>(pt[c]);
    }
  }
}
template void OctreeParser::key2xyz<float>(float* xyz, const unsigned& k, const int d) const;
template void OctreeParser::key2xyz<unsigned>(unsigned* xyz, const unsigned& k, const int d) const;
template void OctreeParser::key2xyz<int>(int* xyz, const unsigned& k, const int d) const;
