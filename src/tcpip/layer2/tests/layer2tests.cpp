// layer2test.cpp

#include "catch2.hpp"
#include "layer2.h"
#include "phy.h"

extern bool phy_frame_buffer_shift_right(uint8_t **pktptr, uint32_t pktlen, uint32_t buflen);

TEST_CASE("Packet buffer shift right with different packet lengths", "[layer2][phy][buffer]") {
  SECTION("Shift right with pktlen=2") {
    uint8_t buf[] = {0xFF, 0xFF, 0x00, 0x00};
    uint8_t *buf_ptr = buf;
    bool result = phy_frame_buffer_shift_right(&buf_ptr, 2, 4);
    REQUIRE(result == true);
    uint8_t buf_exp[] = {0x00, 0x00, 0xFF, 0xFF};
    REQUIRE(memcmp((void *)buf, (void *)buf_exp, 4) == 0);
  }
  SECTION("Shift right with pktlen=3") {
    uint8_t buf[] = {0xFF, 0xFF, 0x00, 0x00};
    uint8_t *buf_ptr = buf;
    bool result = phy_frame_buffer_shift_right(&buf_ptr, 3, 4);
    REQUIRE(result == true);
    uint8_t buf_exp[] = {0x00, 0xFF, 0xFF, 0x00};
    REQUIRE(memcmp((void *)buf, (void *)buf_exp, 4) == 0);
  }
  SECTION("Shift right with pktlen=1") {
    uint8_t buf[] = {0xFF, 0xFF, 0x00, 0x00};
    uint8_t *buf_ptr = buf;
    bool result = phy_frame_buffer_shift_right(&buf_ptr, 1, 4);
    REQUIRE(result == true);
    uint8_t buf_exp[] = {0x00, 0x00, 0x00, 0xFF};
    REQUIRE(memcmp((void *)buf, (void *)buf_exp, 4) == 0);
  }
}

TEST_CASE("Packet buffer shift right boundary conditions", "[layer2][phy][buffer][boundary]") {
  err_logging_disable_guard_t guard; // We expect errors, so silence err logging
  SECTION("Shift with pktlen equal to buflen") {
    uint8_t buf[] = {0xFF, 0xFF, 0x00, 0x00};
    uint8_t *buf_ptr = buf;
    bool result = phy_frame_buffer_shift_right(&buf_ptr, 4, 4);
    REQUIRE(result == true);
    uint8_t buf_exp[] = {0xFF, 0xFF, 0x00, 0x00};
    REQUIRE(memcmp((void *)buf, (void *)buf_exp, 4) == 0);
  }
  SECTION("Shift with pktlen greater than buflen") {
    uint8_t buf[] = {0xFF, 0xFF, 0x00, 0x00};
    uint8_t *buf_ptr = buf;
    bool result = phy_frame_buffer_shift_right(&buf_ptr, 5, 4);
    REQUIRE(result == false); // Illegal
  }
  SECTION("Shift with pktlen=0") {
    uint8_t buf[] = {0xFF, 0xFF, 0x00, 0x00};
    uint8_t *buf_ptr = buf;
    bool result = phy_frame_buffer_shift_right(&buf_ptr, 0, 4);
    REQUIRE(result == false); // Illegal
  }
}

#pragma mark -

// Layer2 qualification tests

static mac_addr_t TEST_MAC_ADDR0 {.bytes = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x01}};
static mac_addr_t TEST_MAC_ADDR1 {.bytes = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x02}};
static mac_addr_t TEST_MAC_BCAST {.bytes = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};

TEST_CASE("L3 Mode", "[layer2][interface][qualify]") {
  err_logging_disable_guard_t guard; // We expect errors, so silence err logging
  // Setup L3 interface
  interface_t intf {
    .netprop = {
      .mode = INTF_MODE_UNKNOWN,
      .mac_addr = TEST_MAC_ADDR0,
      .ip = { .configured = true } // L3 mode
    }
  };
  // Allocate ethernet frame in the stack itself
  uint8_t frame_buf[256] = {0};
  // Tests
  SECTION("Tagged frame on an L3 interface") {
    uint8_t *payload = frame_buf + sizeof(ether_hdr_t) + sizeof(vlan_tag_t);
    ether_hdr_t *hdr = (ether_hdr_t *)(frame_buf + sizeof(vlan_tag_t));
    ether_hdr_set_dst_mac(hdr, &TEST_MAC_ADDR0);
    ether_hdr_set_type(hdr, ETHER_TYPE_IPV4);
    ether_hdr_t *tagged_hdr = ether_hdr_tag_vlan(hdr, sizeof(ether_hdr_t), 100);
    // L3 interfaces should reject tagged frames
    bool resp = layer2_qualify_recv_frame_on_interface(&intf, tagged_hdr);
    REQUIRE(resp == false);
  }
  SECTION("Untagged frame on an L3 interface") {
    uint8_t *payload = frame_buf + sizeof(ether_hdr_t);
    ether_hdr_t *hdr = (ether_hdr_t *)(frame_buf + sizeof(vlan_tag_t));
    ether_hdr_set_type(hdr, ETHER_TYPE_IPV4);
    SECTION("Destination MAC == Interface MAC") {
      ether_hdr_set_dst_mac(hdr, &TEST_MAC_ADDR0);
      // L3 interfaces should accept untagged frames if DST_MAC == INTF_MAC
      bool resp = layer2_qualify_recv_frame_on_interface(&intf, hdr);
      REQUIRE(resp == true);
    }
    SECTION("Destination MAC != Interface MAC") {
      ether_hdr_set_dst_mac(hdr, &TEST_MAC_ADDR1);
      // L3 interfaces should not accept untagged frames if DST_MAC != INTF_MAC
      bool resp = layer2_qualify_recv_frame_on_interface(&intf, hdr);
      REQUIRE(resp == false);
    }
    SECTION("Destination MAC == Broadcast Address") {
      ether_hdr_set_dst_mac(hdr, &TEST_MAC_BCAST);
      // L3 interfaces should accept untagged frames to broadcast address
      bool resp = layer2_qualify_recv_frame_on_interface(&intf, hdr);
      REQUIRE(resp == true);
    }
  }
}

static uint16_t TEST_VLAN_ID_VALID0 = 10;
static uint16_t TEST_VLAN_ID_VALID1 = 20;
static uint16_t TEST_VLAN_ID_INVALID = 30;

TEST_CASE("L2 ACCESS Mode", "[layer2][interface][qualify]") {
  err_logging_disable_guard_t guard; // We expect errors, so silence err logging
  // Setup L2 interface
  interface_t intf;
  interface_enable_l2_mode(&intf, INTF_MODE_L2_ACCESS);
  // Allocate ethernet frame in the stack itself
  uint8_t frame_buf[256] = {0};
  SECTION("Tagged frame") {
    // Create tagged header
    uint8_t *payload = frame_buf + sizeof(ether_hdr_t) + sizeof(vlan_tag_t);
    ether_hdr_t *hdr = (ether_hdr_t *)(frame_buf + sizeof(vlan_tag_t));
    ether_hdr_set_dst_mac(hdr, &TEST_MAC_ADDR0);
    ether_hdr_set_type(hdr, ETHER_TYPE_IPV4);
    ether_hdr_t *tagged_hdr = ether_hdr_tag_vlan(hdr, sizeof(ether_hdr_t), TEST_VLAN_ID_VALID0);
    // Cases
    SECTION("No assigned VLANs") {
      interface_clear_l2_vlan_memberships(&intf);
      // L2 ACCESS mode without any VLAN assignment should reject all ingress frames
      bool resp = layer2_qualify_recv_frame_on_interface(&intf, tagged_hdr);
      REQUIRE(resp == false);
    }
    SECTION("Matches port VLAN") {
      interface_clear_l2_vlan_memberships(&intf);
      interface_add_l2_vlan_membership(&intf, TEST_VLAN_ID_VALID0);
      // L2 ACCESS mode should allow ingress frames with matching VLAN ID
      bool resp = layer2_qualify_recv_frame_on_interface(&intf, tagged_hdr);
      REQUIRE(resp == true);
    }
    SECTION("Does not match port VLAN") {
      interface_clear_l2_vlan_memberships(&intf);
      interface_add_l2_vlan_membership(&intf, TEST_VLAN_ID_INVALID);
      SECTION("0th VLAN ID doesn't match") {
        // L2 ACCESS mode should not allow ingress frames with different VLAN ID
        bool resp = layer2_qualify_recv_frame_on_interface(&intf, tagged_hdr);
        REQUIRE(resp == false);
      }
      SECTION("0th VLAN ID doesn't match but Nth does") {
        // We manually add VLAN ID because interface_add_l2_vlan_membership won't
        // allow us to add more than one VLANs in L2 ACCESS mode.
        intf.netprop.vlan_memberships[1] = TEST_VLAN_ID_VALID0;
        // L2 ACCESS mode should not allow ingress frames with different VLAN ID
        bool resp = layer2_qualify_recv_frame_on_interface(&intf, tagged_hdr);
        REQUIRE(resp == false);
      }
    }
  }
  SECTION("Untagged frame") {
    // Create untagged header
    uint8_t *payload = frame_buf + sizeof(ether_hdr_t);
    ether_hdr_t *hdr = (ether_hdr_t *)(frame_buf);
    ether_hdr_set_dst_mac(hdr, &TEST_MAC_ADDR0);
    ether_hdr_set_type(hdr, ETHER_TYPE_IPV4);
    // Cases
    SECTION("No assigned VLANs") {
      interface_clear_l2_vlan_memberships(&intf);
      // L2 ACCESS mode without any VLAN assignment should reject all ingress frames
      bool resp = layer2_qualify_recv_frame_on_interface(&intf, hdr);
      REQUIRE(resp == false);
    }
    SECTION("Has assigned VLAN") {
      interface_clear_l2_vlan_memberships(&intf);
      interface_add_l2_vlan_membership(&intf, TEST_VLAN_ID_VALID0);
      // L2 ACCESS mode with assigned VLANs should accept (and ultimately tag)
      // untagged frames (tagging is out of scope for this test/function).
      bool resp = layer2_qualify_recv_frame_on_interface(&intf, hdr);
      REQUIRE(resp == true);
    }
  }
}

TEST_CASE("L2 TRUNK Mode", "[layer2][interface][qualify]") {
  err_logging_disable_guard_t guard; // We expect errors, so silence err logging
  // Setup L2 interface
  interface_t intf;
  interface_enable_l2_mode(&intf, INTF_MODE_L2_TRUNK);
  // Allocate ethernet frame in the stack itself
  uint8_t frame_buf[256] = {0};
  SECTION("Tagged frame") {
    // Create tagged header
    uint8_t *payload = frame_buf + sizeof(ether_hdr_t) + sizeof(vlan_tag_t);
    ether_hdr_t *hdr = (ether_hdr_t *)(frame_buf + sizeof(vlan_tag_t));
    ether_hdr_set_dst_mac(hdr, &TEST_MAC_ADDR0);
    ether_hdr_set_type(hdr, ETHER_TYPE_IPV4);
    ether_hdr_t *tagged_hdr = ether_hdr_tag_vlan(hdr, sizeof(ether_hdr_t), TEST_VLAN_ID_VALID0);
    // Cases
    SECTION("No assigned VLANs") {
      interface_clear_l2_vlan_memberships(&intf);
      // L2 ACCESS mode without any VLAN assignment should reject all ingress frames
      bool resp = layer2_qualify_recv_frame_on_interface(&intf, tagged_hdr);
      REQUIRE(resp == false);
    }
    SECTION("Matches port VLAN") {
      interface_clear_l2_vlan_memberships(&intf);
      SECTION("Matches first") {
        interface_add_l2_vlan_membership(&intf, TEST_VLAN_ID_VALID0);
        // L2 ACCESS mode should allow ingress frames with matching VLAN ID
        bool resp = layer2_qualify_recv_frame_on_interface(&intf, tagged_hdr);
        REQUIRE(resp == true);
      }
      SECTION("Matches Nth") {
        interface_add_l2_vlan_membership(&intf, TEST_VLAN_ID_VALID1);
        interface_add_l2_vlan_membership(&intf, TEST_VLAN_ID_VALID0);
        // L2 ACCESS mode should allow ingress frames with matching VLAN ID
        bool resp = layer2_qualify_recv_frame_on_interface(&intf, tagged_hdr);
        REQUIRE(resp == true);
      }
    }
    SECTION("Does not match port VLAN") {
      interface_clear_l2_vlan_memberships(&intf);
      interface_add_l2_vlan_membership(&intf, TEST_VLAN_ID_INVALID);
      SECTION("0th VLAN ID doesn't match") {
        // L2 ACCESS mode should not allow ingress frames with different VLAN ID
        bool resp = layer2_qualify_recv_frame_on_interface(&intf, tagged_hdr);
        REQUIRE(resp == false);
      }
    }
  }
  SECTION("Untagged frame") {
    // Create untagged header
    uint8_t *payload = frame_buf + sizeof(ether_hdr_t);
    ether_hdr_t *hdr = (ether_hdr_t *)(frame_buf);
    ether_hdr_set_dst_mac(hdr, &TEST_MAC_ADDR0);
    ether_hdr_set_type(hdr, ETHER_TYPE_IPV4);
    // Cases
    SECTION("No assigned VLANs") {
      interface_clear_l2_vlan_memberships(&intf);
      // L2 TRUNK mode without any VLAN assignment should reject all ingress frames
      bool resp = layer2_qualify_recv_frame_on_interface(&intf, hdr);
      REQUIRE(resp == false);
    }
    SECTION("Has assigned VLAN") {
      interface_clear_l2_vlan_memberships(&intf);
      interface_add_l2_vlan_membership(&intf, TEST_VLAN_ID_VALID0);
      // L2 TRUNK mode with assigned VLANs should not accept any untagged frames.
      bool resp = layer2_qualify_recv_frame_on_interface(&intf, hdr);
      REQUIRE(resp == false);
    }
  }
}

TEST_CASE("L2 UNKNOWN Mode", "[layer2][interface][qualify]") {
  err_logging_disable_guard_t guard; // We expect errors, so silence err logging
  // Setup L2 interface
  interface_t intf;
  interface_enable_l2_mode(&intf, INTF_MODE_UNKNOWN);
  // Allocate ethernet frame in the stack itself
  uint8_t frame_buf[256] = {0};
  SECTION("Tagged frame") {
    // Create tagged header
    uint8_t *payload = frame_buf + sizeof(ether_hdr_t) + sizeof(vlan_tag_t);
    ether_hdr_t *hdr = (ether_hdr_t *)(frame_buf + sizeof(vlan_tag_t));
    ether_hdr_set_dst_mac(hdr, &TEST_MAC_ADDR0);
    ether_hdr_set_type(hdr, ETHER_TYPE_IPV4);
    ether_hdr_t *tagged_hdr = ether_hdr_tag_vlan(hdr, sizeof(ether_hdr_t), TEST_VLAN_ID_VALID0);
    // UNKNOWN mode must reject all ingress frames
    bool resp = layer2_qualify_recv_frame_on_interface(&intf, tagged_hdr);
    REQUIRE(resp == false);
  }
  SECTION("Untagged frame") {
    // Create untagged header
    uint8_t *payload = frame_buf + sizeof(ether_hdr_t);
    ether_hdr_t *hdr = (ether_hdr_t *)(frame_buf);
    ether_hdr_set_dst_mac(hdr, &TEST_MAC_ADDR0);
    ether_hdr_set_type(hdr, ETHER_TYPE_IPV4);
    // UNKNOWN mode must reject all ingress frames
    bool resp = layer2_qualify_recv_frame_on_interface(&intf, hdr);
    REQUIRE(resp == false);
  }
}
