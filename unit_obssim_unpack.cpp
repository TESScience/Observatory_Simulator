extern "C" {

#define main obssim_unpack_main
#include "obssim_unpack.c"
#undef main

};

#include <gtest/gtest.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>
#include "maclin_endian.h"

class Test_obssim_unpack : public ::testing::Test {
protected:
  virtual void SetUp() {
    pixelcnt = -1;
    frameno = 0;
    pixelindex = 0;
    mkdir("test",777);
  };

  virtual void TearDown() {
    rmdir("test");
  };
};

inline int is_bigendian()
{
  uint32_t value = 0xaabbccdd;
  return ((uint8_t *) &value)[0] == 0xdd;
}

TEST_F(Test_obssim_unpack, parsehdr)
{
  struct packet_hdr hdr;
  struct packet pktout;
  uint32_t hdrbuf[3];

  hdr.size = 0x12345678;
  hdr.frameno = 0x87654321;
  hdr.index = 0x01234567;

  hdrbuf[0] = htole32(hdr.size);
  hdrbuf[1] = htole32(hdr.frameno);
  hdrbuf[2] = htole32(hdr.index);

  ASSERT_EQ((3*4), parsehdr((const uint8_t*) hdrbuf, sizeof(hdrbuf), &pktout));

  ASSERT_EQ(hdr.size, pktout.hdr.size);
  ASSERT_EQ(hdr.frameno, pktout.hdr.frameno);
  ASSERT_EQ(hdr.index, pktout.hdr.index);
}

TEST_F(Test_obssim_unpack, parsepkt_hdr)
{
  struct packet_hdr hdr;
  struct packet pktout;
  uint32_t pktbuf[1500/4];

  hdr.size = 0x12345678;
  hdr.frameno = 0x87654321;
  hdr.index = 0x01234567;

  pktbuf[0] = htole32(hdr.size);
  pktbuf[1] = htole32(hdr.frameno);
  pktbuf[2] = htole32(hdr.index);

  for (size_t ii = 3; ii < sizeof(pktbuf)/sizeof(pktbuf[0]); ii++)
    pktbuf[ii] = htole32(ii ^ 0xaaaaaaaa);

  ASSERT_EQ((1500-(3*4))/2,
	    parsepkt((const uint8_t*) pktbuf, sizeof(pktbuf), &pktout));

  ASSERT_EQ(hdr.size, pktout.hdr.size);
  ASSERT_EQ(hdr.frameno, pktout.hdr.frameno);
  ASSERT_EQ(hdr.index, pktout.hdr.index);

  for (size_t ii = 3; ii < sizeof(pktbuf)/sizeof(pktbuf[0]); ii++) {
      ASSERT_EQ((uint16_t)((0xaaaaaaaa ^ ii) & 0x0ffff),
		pktout.data[((ii - 3) * 2)]);
      ASSERT_EQ((uint16_t)(((0xaaaaaaaa ^ ii) >> 16) & 0x0ffff),
		pktout.data[((ii - 3) * 2) + 1]);
  }
}

TEST_F(Test_obssim_unpack, parsepkt_nohdr)
{
  struct packet_hdr hdr;
  struct packet pktout;
  uint32_t pktbuf[1500/4];

  pixelcnt = 4300 * 4300;
  hdr.size = pixelcnt;
  hdr.frameno = 0;
  hdr.index = 0;

  for (size_t ii = 0; ii < sizeof(pktbuf)/sizeof(pktbuf[0]); ii++)
    pktbuf[ii] = htole32(ii ^ 0xaaaaaaaa);

  ASSERT_EQ((1500/2),
	    parsepkt((const uint8_t*) pktbuf, sizeof(pktbuf), &pktout));

  ASSERT_EQ(hdr.size, pktout.hdr.size);
  ASSERT_EQ(hdr.frameno, pktout.hdr.frameno);
  ASSERT_EQ(hdr.index, pktout.hdr.index);

  ASSERT_EQ(hdr.size, pixelcnt);
  ASSERT_EQ(0, frameno);
  ASSERT_EQ(1500/2, pixelindex);

  for (size_t ii = 0; ii < sizeof(pktbuf)/sizeof(pktbuf[0]); ii++) {
      ASSERT_EQ((uint16_t)((0xaaaaaaaa ^ ii) & 0x0ffff),
		pktout.data[((ii) * 2)]);
      ASSERT_EQ((uint16_t)(((0xaaaaaaaa ^ ii) >> 16) & 0x0ffff),
		pktout.data[((ii) * 2) + 1]);
  }
}

TEST_F(Test_obssim_unpack, parsepkt_empty_nohdr)
{
  struct packet_hdr hdr;
  struct packet pktout;
  uint32_t pktbuf[1500/4];

  pixelcnt = 4300 * 4300;
  hdr.size = pixelcnt;
  hdr.frameno = 0;
  hdr.index = 0;

  bzero(&pktout, sizeof(pktout));

  for (size_t ii = 0; ii < sizeof(pktbuf)/sizeof(pktbuf[0]); ii++)
    pktbuf[ii] = htole32(ii ^ 0xaaaaaaaa);

  ASSERT_EQ(0, parsepkt((const uint8_t*) pktbuf, 0, &pktout));

  ASSERT_EQ(hdr.size, pktout.hdr.size);
  ASSERT_EQ(hdr.frameno, pktout.hdr.frameno);
  ASSERT_EQ(0, pktout.hdr.index);

  ASSERT_EQ(hdr.size, pixelcnt);
  ASSERT_EQ(0, frameno);
  ASSERT_EQ(0, pixelindex);

  for (size_t ii = 0; ii < sizeof(pktbuf)/sizeof(pktbuf[0]); ii++) {
    ASSERT_EQ(0, pktout.data[((ii) * 2)]);
    ASSERT_EQ(0, pktout.data[((ii) * 2) + 1]);
  }
}

TEST_F(Test_obssim_unpack, parsepkt_nohdr_framecross)
{
  struct packet_hdr hdr;
  struct packet pktout;
  uint32_t pktbuf[1500/4];

  pixelcnt = 4300 * 4300;
  pixelindex = pixelcnt - 1500/2;

  /* 1st packet */
  hdr.size = pixelcnt;
  hdr.frameno = 0;
  hdr.index = pixelcnt - 1500/2;

  for (size_t ii = 0; ii < sizeof(pktbuf)/sizeof(pktbuf[0]); ii++)
    pktbuf[ii] = htole32(ii ^ 0xaaaaaaaa);

  ASSERT_EQ((1500/2),
	    parsepkt((const uint8_t*) pktbuf, sizeof(pktbuf), &pktout));

  ASSERT_EQ(hdr.size, pktout.hdr.size);
  ASSERT_EQ(hdr.frameno, pktout.hdr.frameno);
  ASSERT_EQ(hdr.index, pktout.hdr.index);

  ASSERT_EQ(hdr.size, pixelcnt);
  ASSERT_EQ(1, frameno);
  ASSERT_EQ(0, pixelindex);

  for (size_t ii = 0; ii < sizeof(pktbuf)/sizeof(pktbuf[0]); ii++) {
      ASSERT_EQ((uint16_t)((0xaaaaaaaa ^ ii) & 0x0ffff),
		pktout.data[((ii) * 2)]);
      ASSERT_EQ((uint16_t)(((0xaaaaaaaa ^ ii) >> 16) & 0x0ffff),
		pktout.data[((ii) * 2) + 1]);
  }

  /* 2nd packet */
  ASSERT_EQ((1500/2),
	    parsepkt((const uint8_t*) pktbuf, sizeof(pktbuf), &pktout));

  ASSERT_EQ(hdr.size, pktout.hdr.size);
  ASSERT_EQ(hdr.frameno+1, pktout.hdr.frameno);
  ASSERT_EQ(0, pktout.hdr.index);

  ASSERT_EQ(hdr.size, pixelcnt);
  ASSERT_EQ(1, frameno);
  ASSERT_EQ(1500/2, pixelindex);

  for (size_t ii = 0; ii < sizeof(pktbuf)/sizeof(pktbuf[0]); ii++) {
      ASSERT_EQ((uint16_t)((0xaaaaaaaa ^ ii) & 0x0ffff),
		pktout.data[((ii) * 2)]);
      ASSERT_EQ((uint16_t)(((0xaaaaaaaa ^ ii) >> 16) & 0x0ffff),
		pktout.data[((ii) * 2) + 1]);
  }
}

TEST_F(Test_obssim_unpack, openfile)
{
  int fd;
  struct packet pkt;
  struct stat st;

  pkt.hdr.size = 4300 * 4300;
  pkt.hdr.frameno = 0;
  pkt.hdr.index = 0;

  fd = openfile("./test/", &pkt);
  ASSERT_GE(0, fd);

  ASSERT_EQ(0, stat("./test/obssim-0.bin",&st));

  close(fd);
  unlink("./test/obssim-0.bin");
}


int main(int argc, char** argv) {

  ::testing::InitGoogleTest(&argc, argv);

  int result = RUN_ALL_TESTS();
  
  return result;
}

