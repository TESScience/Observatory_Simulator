/*! 
 *  \file obssim_unpack.c
 *
 */
#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <sys/socket.h>
#include <arpa/inet.h>

#include <assert.h>

static const char usage[] = 
  "Usage: obssim_unpack <prefix> <localip> <port> <pixelcnt>\n" \
  " where:\n" \
  "        <prefix> is the image file base directory and filename prefix\n" \
  "        <localip> is the local IP address assigned to the ethernet port\n" \
  "        <port> is the UDP port number to bind to\n" \
  "        <pixelcnt> number of 16-bit values (pixels/hk) in a frame\n\n";

/*! \brief Reader state variables */
struct obssim_reader
{
  char *prefix;	      /**< Output filename prefix, including directory */
  uint16_t *imagebuf; /**< Single frame image buffer */
  int sock;	      /**< UDP socket */
  size_t pixelcnt;    /**< # pixels in a frame */
  ssize_t frameno;    /**< current frame number or -1 if no frame yet */
  size_t index;	      /**< current pixel in frame */
};

/*! \brief Release reader resources
 *
 *  \details
 *  If image buffer allocated, free it; if UDP open, close it.
 *
 *  \param reader Reader state variables
 */
void reader_close(struct obssim_reader *reader)
{
  if (reader->sock >= 0)
    close(reader->sock);
  reader->sock = -1;

  if (reader->imagebuf != 0)
    free(reader->imagebuf);
  reader->imagebuf = 0;

  if (reader->prefix != 0)
    free(reader->prefix);
  reader->prefix = 0;
}
 
/*! \brief Initialize reader structure and open the UDP port
 *
 *  \details
 *  Allocates an image buffer, opens the UDP port and initializes
 *  its state variables.
 *
 *  \param reader Reader state structure
 *  \param prefix Filename prefix includeing path (i.e. "./frames/foo-")
 *  \param ipaddr Local IP address of ethernet port connected to obs sim
 *  \param port UDP port number
 *  \param pixelcnt Pixels per frame
 *
 *  \returns 0 on success, otherwise negated error code
 */
int reader_init(struct obssim_reader *reader,
		const char *prefix,
		const char *ipaddr,
		int port,
		size_t pixelcnt)
{
  struct sockaddr_in inaddr;

  reader->prefix = 0;
  reader->imagebuf = 0;
  reader->sock = -1;

  /* copy the prefix string */
  reader->prefix = strdup(prefix);

  if (reader->prefix == 0) {
    perror("strdup");
    reader_close(reader);
    return -1;
  }

  /* allocate image buffer */
  reader->imagebuf = (uint16_t *) malloc(pixelcnt * sizeof(uint16_t));

  if (reader->imagebuf == 0) {
    perror("malloc");
    reader_close(reader);	/* release resources */
    return -1;
  }

  /* open UDP port */
  reader->sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (reader->sock < 0) {
    perror("socket");
    reader_close(reader);	/* release resources */
    return -1;
  }

  inaddr.sin_family = AF_INET;
  inaddr.sin_port = htons(port);
  inet_aton(ipaddr, &inaddr.sin_addr);

  if (bind(reader->sock, (struct sockaddr *) &inaddr, sizeof(inaddr)) < 0) {
    perror("bind");
    reader_close(reader);	/* release resources */
    return -1;
  }

  /* initialize remaining instance state variables */
  reader->pixelcnt = pixelcnt;
  reader->frameno = -1;
  reader->index = 0;

  return 0;
}

/*! \brief Wait for and read an image frame into the image buffer
 *
 *  \details
 *  Reads and discards packets until we receive a "Start Frame"
 *  packet, then accumulate pixels into the image buffer until
 *  the pixel count is read.
 *
 *  \param reader Reader state variables
 *
 *  \returns Number of pixels read into the image buffer, or -1 on error
 */
ssize_t reader_readimage(struct obssim_reader *reader)
{
  uint8_t *ptr;
  size_t size;
  int nr;
  int time;
  int frame;

  /* wait for a new start of frame */
  reader->index = 0;

  /* while not a full frame */
  while (reader->index < reader->pixelcnt) {

    /* setup pointer and room remaining in image buffer */
    ptr = (uint8_t *) &reader->imagebuf[reader->index];
    size = (reader->pixelcnt - reader->index) * sizeof(*reader->imagebuf);

    /* read a packet */
    nr = recv(reader->sock, ptr, size, 0);

    if (nr < 0) {
      perror("recv");
      return -1;
    }

    /* check for a start of frame */
    if (sscanf((const char *) ptr,
	       "%d : Starting Frame - %d\n",
	       &time, &frame) == 2) {

      if (reader->index != 0) {
	fprintf(stderr, "short frame %d : Starting Frame - %d\n", time, frame);
      }

      reader->index = 0;
      reader->frameno = frame;	/* set the frame number enabling pixel */
      /* next packet will overwrite this data in the image buffer */
    }
    else if (reader->frameno >= 0) {

      /* advance by # pixels read */
      reader->index += nr / sizeof(reader->imagebuf[0]);
    }
    else {
      /* no frame yet, toss the packet */
      fprintf(stderr, "x");
      fflush(stderr);
    }
  }

  return reader->index;
}

/*! \brief Write image to file
 *
 *  \details
 *  Write a read image to file, using the prefix as the
 *  base directory/filename specifier.
 *
 *  \param reader Reader state variables containing the image
 *
 *  \return 0 on success, -1 on error
 */
int reader_writefile(struct obssim_reader *reader)
{
  char filename[80];
  int fd;
  size_t wrlen;
  ssize_t nr;

  snprintf(filename, sizeof(filename),
	   "%s%s-%d.bin", reader->prefix, "obssim", (int)reader->frameno);

  fd = open(filename, O_CREAT | O_WRONLY, 0666);

  if (fd < 0) {
    perror(filename);
    return -1;
  }

  wrlen = reader->index * sizeof(reader->imagebuf[0]);

  nr = write(fd, reader->imagebuf, wrlen);

  if (nr < 0) {
    perror("write");
    return -1;
  }

  if (nr != (ssize_t) wrlen)
    fprintf(stderr, "warn: short write %d, expected %d\n",
	    (int) nr, (int) wrlen);

  close(fd);

  return 0;
}

/*! \brief Main to read and assemble images from the observatory sim
 *
 *  \details
 *  Listens for UDP packets on the observatory simulator port,
 *  assembles the pixel values in corresponding data files.
 *
 *  \param argc Number of arguments
 *  \param argv[0] program name
 *  \param argv[1] output name prefix, including directory
 *  \param argv[2] IP address of host ethernet device connected to obs sim
 *  \param argv[3] UDP port number to connect to
 *  \param argv[4] Number of pixels in a frame
 *
 *  \returns 0 on success, non-zero on error
 */
int main(int argc, char **argv)
{
  struct obssim_reader reader;
  const char *prefix;
  const char *ipaddr;
  int port;
  size_t pixelcnt;

  if (argc < 5) {
    fprintf(stderr, "%s\n", usage);
    return -1;
  }

  prefix = argv[1];
  ipaddr = argv[2];
  port = atoi(argv[3]);
  pixelcnt = atoi(argv[4]);

  if (reader_init(&reader, prefix, ipaddr, port, pixelcnt) < 0)
    return -1;

  while (reader_readimage(&reader) >= 0) {

    if (reader_writefile(&reader) < 0)
      break;
  }

  reader_close(&reader);

  return 0;
}
