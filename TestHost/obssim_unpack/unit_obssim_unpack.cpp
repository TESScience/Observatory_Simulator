extern "C" {

#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/socket.h>
#include "maclin_endian.h"

#define main obssim_unpack_main

#define write stub_write
  ssize_t stub_write(int fd, const void *buf, size_t nbyte);

#define socket stub_socket
  int stub_socket(int domain, int type, int protocol);

#define recv stub_recv
     ssize_t stub_recv(int socket, void *buffer, size_t length, int flags);

#include "obssim_unpack.c"
#undef main
#undef write
#undef socket

};

#include <gtest/gtest.h>

ssize_t stub_writeerr = 0;
ssize_t stub_write(int fd, const void *buf, size_t nbyte)
{
  if (stub_writeerr == 0)
    return write(fd, buf, nbyte);
  else
    return stub_writeerr;
}

ssize_t stub_socketerr = 0;
int stub_socket(int domain, int type, int protocol)
{
  if (stub_socketerr == 0)
    return socket(domain, type, protocol);
  else
    return stub_socketerr;
}

int stub_recvflag = 0;
ssize_t stub_recverr[3] = { -1, -1, -1 };

ssize_t stub_recv(int socket, void *buffer, size_t length, int flags)
{
  if (stub_recvflag > 0)
    return stub_recverr[stub_recvflag--];
  else
    return recv(socket, buffer, length, flags);
}

class Test_obssim_unpack : public ::testing::Test {
protected:
  virtual void SetUp() {
    pixelcnt = -1;
    frameno = 0;
    pixelindex = 0;
    mkdir("test",0777);
    
    stub_writeerr = 0;
    stub_socketerr = 0;
    stub_recvflag = 0;
  };

  virtual void TearDown() {
    unlink("./test/obssim-0.bin");
    unlink("./test/obssim-1.bin");
    rmdir("test");
  };

  void filematch(const char *filename,
		 const uint8_t *buf,
		 size_t buflen)
  {
    FILE *fp = fopen(filename, "r");
    ASSERT_NE((FILE *) 0, fp);

    for (size_t ii = 0; ii < buflen; ii++)
      if (buf[ii] != fgetc(fp))
	fprintf(stderr, "Mismatch byte %u\n", (unsigned int) ii);

    rewind(fp);

    while (buflen--)
      ASSERT_EQ(*buf++, fgetc(fp));

    ASSERT_EQ(-1, fgetc(fp));
    fclose(fp);
  }
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
  ASSERT_LE(0, fd);

  ASSERT_EQ(0, stat("./test/obssim-0.bin",&st));

  close(fd);
  unlink("./test/obssim-0.bin");
}


TEST_F(Test_obssim_unpack, storepkt_ok)
{
  const char *base = "./test/";
  struct packet pkt;
  size_t datacnt;
  uint32_t lastframe;
  int lastfd;
  off_t lastpos;

  pkt.hdr.size = 4300 * 4300;
  pkt.hdr.frameno = 0;
  pkt.hdr.index = 0;

  datacnt = sizeof(pkt.data)/sizeof(uint16_t);
  lastfd = -1;
  lastframe = 0;
  lastpos = 0;

  for (size_t ii = 0; ii < datacnt; ii++)
    pkt.data[ii] = 0xaaaa ^ ii;

  storepkt(base, &pkt, datacnt, &lastframe, &lastfd, &lastpos);

  ASSERT_LE(0, lastfd);
  ASSERT_EQ(0, lastframe);
  filematch("./test/obssim-0.bin",
	    (const uint8_t *) pkt.data,
	    sizeof(pkt.data));

  unlink("./test/obssim-0.bin");
}

TEST_F(Test_obssim_unpack, storepkt_firstframe)
{
  const char *base = "./test/";
  struct packet pkt;
  size_t datacnt;
  uint32_t lastframe;
  int lastfd;
  off_t lastpos;

  pkt.hdr.size = 4300 * 4300;
  pkt.hdr.frameno = 0;
  pkt.hdr.index = 0;

  /* build a full image to store */
  uint16_t *image = new uint16_t[pkt.hdr.size];

  for (size_t ii = 0; ii < (4300 * 4300); ii++)
    image[ii] = 0xaaaa ^ ii;

  datacnt = sizeof(pkt.data)/sizeof(uint16_t);
  lastfd = -1;
  lastpos = 0;
  lastframe = 0;

  /* send the image one packet at a time */
  for (size_t ii = 0; ii < (4300 * 4300); ii += datacnt) {

    memcpy(pkt.data, image+ii, sizeof(pkt.data));

    pkt.hdr.index = ii;

    if ((ii + datacnt) > (4300 * 4300))
      datacnt = (4300 * 4300) - ii;

    storepkt(base, &pkt, datacnt, &lastframe, &lastfd, &lastpos);
  }
  
  filematch("./test/obssim-0.bin",
	    (const uint8_t *) image,
	    pkt.hdr.size * sizeof(uint16_t));

  /* send a second image */
  pkt.hdr.frameno++;

  for (size_t ii = 0; ii < (4300 * 4300); ii += datacnt) {

    memcpy(pkt.data, image+ii, sizeof(pkt.data));

    pkt.hdr.index = ii;

    if ((ii + datacnt) > (4300 * 4300))
      datacnt = (4300 * 4300) - ii;

    storepkt(base, &pkt, datacnt, &lastframe, &lastfd, &lastpos);
  }

  filematch("./test/obssim-1.bin",
	    (const uint8_t *) image,
	    pkt.hdr.size * sizeof(uint16_t));

  unlink("./test/obssim-0.bin");
  unlink("./test/obssim-1.bin");

  delete [] image;
}

TEST_F(Test_obssim_unpack, storepkt_openseekerr)
{
  const char *base = "./test/";
  struct packet pkt;
  size_t datacnt;
  uint32_t lastframe;
  int lastfd;
  off_t lastpos;

  pkt.hdr.size = 4300 * 4300;
  pkt.hdr.frameno = 0;
  pkt.hdr.index = 0;

  datacnt = sizeof(pkt.data)/sizeof(uint16_t);
  lastfd = -1;
  lastframe = 0;
  lastpos = 10;

  for (size_t ii = 0; ii < datacnt; ii++)
    pkt.data[ii] = 0xaaaa ^ ii;

  /* force an open and subsequent seek error by
   * deleting the parent directory
   */
  rmdir("test");
  storepkt(base, &pkt, datacnt, &lastframe, &lastfd, &lastpos);

  ASSERT_GT(0, lastfd);
}

TEST_F(Test_obssim_unpack, storepkt_writeerr)
{
  const char *base = "./test/";
  struct packet pkt;
  size_t datacnt;
  uint32_t lastframe;
  int lastfd;
  off_t lastpos;

  pkt.hdr.size = 4300 * 4300;
  pkt.hdr.frameno = 0;
  pkt.hdr.index = 0;

  datacnt = sizeof(pkt.data)/sizeof(uint16_t);
  lastfd = -1;
  lastpos = 0;
  lastframe = 0;

  for (size_t ii = 0; ii < datacnt; ii++)
    pkt.data[ii] = 0xaaaa ^ ii;

  /* force a write error */
  stub_writeerr = -1;

  storepkt(base, &pkt, datacnt, &lastframe, &lastfd, &lastpos);

  ASSERT_GT(0, lastfd);
}

TEST_F(Test_obssim_unpack, udpopen_ok)
{
  int fd = -1;

  fd = udpopen("127.0.0.1", 5555);
  ASSERT_LE(0, fd);

  close(fd);
}

TEST_F(Test_obssim_unpack, udpopen_sockerr)
{
  int fd = -1;

  stub_socketerr = -1;

  fd = udpopen("127.0.0.1",5555);
  ASSERT_EQ(-1, fd);
}

TEST_F(Test_obssim_unpack, udpopen_binderr)
{
  int fd1 = -1;
  int fd2 = -1;

  fd1 = udpopen("127.0.0.1",5555);
  ASSERT_LE(0, fd1);

  fd2 = udpopen("127.0.0.1",5555);
  ASSERT_EQ(-1, fd2);

  close(fd1);
}

TEST_F(Test_obssim_unpack, readimages)
{
  //  FAIL();
}

TEST_F(Test_obssim_unpack, readimages_nodata)
{
  //  FAIL();
}

TEST_F(Test_obssim_unpack, readimages_readerr)
{
  const char base[] = "./test/";
  int sock;

  pixelcnt = 4300 * 4300;
  sock = udpopen("127.0.0.1",5555);

  stub_recvflag = 1;
  stub_recverr[1] = -1;

  readimages(base, sock);

  close(sock);
}

TEST_F(Test_obssim_unpack, main_ok)
{
  //  FAIL();
}

TEST_F(Test_obssim_unpack, main_usage)
{
  //  FAIL();
}

TEST_F(Test_obssim_unpack, main_nohdr)
{
  //  FAIL();
}

TEST_F(Test_obssim_unpack, main_udperr)
{
  //  FAIL();
}

int main(int argc, char** argv) {

  ::testing::InitGoogleTest(&argc, argv);

  int result = RUN_ALL_TESTS();
  
  return result;
}

