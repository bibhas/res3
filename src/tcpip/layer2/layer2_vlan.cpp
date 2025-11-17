// layer2_vlan.cpp

#include <arpa/inet.h>
#include "layer3/layer3.h"
#include "layer2.h"
#include "graph.h"
#include "mac.h"
#include "phy.h"
#include "pcap.h"
#include "vlan_tag.h"
#include "ether_hdr.h"

ether_hdr_t* ether_hdr_tag_vlan(ether_hdr_t *hdr, uint32_t len, uint16_t vlanid, uint32_t *newlen) {
  EXPECT_RETURN_VAL(hdr != nullptr, "Empty header ptr param", nullptr);
  // First, check if frame is tagged
  if (ether_hdr_read_type(hdr) == ETHER_TYPE_VLAN) {
    // No need to do anything
    if (newlen) {
      *newlen = len;
    }
    return hdr;
  }
  // Allocate a dedicated static temp buffer for vlan tagging
  static ether_hdr_t *temp_hdr = nullptr;
  if (temp_hdr == nullptr) {
    temp_hdr = (ether_hdr_t *)malloc(sizeof(ether_hdr_t));
  }
  // Copy just the ethernet hdr to temp
  memcpy(temp_hdr, hdr, sizeof(ether_hdr_t));
  // Find start of payload
  uint8_t *payload = (uint8_t *)(hdr + 1);
  // Locate vlan header start ptr
  uint8_t *_tag = (payload - sizeof(vlan_tag_t));
  uint8_t *_new_hdr = (_tag - sizeof(ether_hdr_t));
  vlan_tag_t *tag = (vlan_tag_t *)_tag;
  vlan_tag_init(tag);
  ether_hdr_t *new_hdr = (ether_hdr_t *)_new_hdr;
  ether_hdr_init(new_hdr);
  // Copy over temp to new header
  memcpy(new_hdr, temp_hdr, sizeof(ether_hdr_t));
  // Fill in vland id in the tag
  vlan_tag_set_vlan_id(tag, vlanid);
  // Copy ether_type to type
  vlan_tag_set_ether_type(tag, ether_hdr_read_type(temp_hdr));
  // Set frame type to VLAN
  ether_hdr_set_type(new_hdr, ETHER_TYPE_VLAN);
  // And, we're done
  if (newlen) {
    *newlen = len + sizeof(vlan_tag_t);
  }
  return new_hdr;
}

ether_hdr_t* ether_hdr_untag_vlan(ether_hdr_t *hdr, uint32_t len, uint32_t *newlen) {
  EXPECT_RETURN_VAL(hdr != nullptr, "Empty header ptr param", nullptr);
  // First, check if frame is tagged at all
  if (ether_hdr_read_type(hdr) != ETHER_TYPE_VLAN) {
    // No need to do anything
    *newlen = len;
    return hdr;
  }
  // Allocate a dedicated static temp buffer for vlan untagging
  static ether_hdr_t *temp_hdr = nullptr;
  if (temp_hdr == nullptr) {
    temp_hdr = (ether_hdr_t *)malloc(sizeof(ether_hdr_t));
  }
  // Copy header to temp -> zero out memory
  memcpy((void *)temp_hdr, (void *)hdr, sizeof(ether_hdr_t));
  memset((void *)hdr, 0, sizeof(ether_hdr_t));
  // Retrieve ethertype of VLAN
  vlan_tag_t *tag = (vlan_tag_t *)(hdr + 1);
  uint16_t orig_type = vlan_tag_read_ether_type(tag);
  // Then, zero out vlan 
  memset((void *)tag, 0, sizeof(vlan_tag_t));
  // Locate new header ptr
  uint8_t *payload = (uint8_t *)(tag + 1);
  uint8_t *_new_hdr = payload - sizeof(ether_hdr_t);
  ether_hdr_t *new_hdr = (ether_hdr_t *)_new_hdr;
  // Copy over header from temp
  memcpy((void *)new_hdr, (void *)temp_hdr, sizeof(ether_hdr_t));
  // Update ether type
  ether_hdr_set_type(new_hdr, orig_type);
  // And, we're done
  *newlen = len - sizeof(vlan_tag_t);
  return new_hdr;
}

