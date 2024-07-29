#include <liburing.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define QUEUE_DEPTH 2
#define BLOCK_SIZE_DEMO 4096

void handle_error(const char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <source_file> <destination_file>\n", argv[0]);
        exit(1);
    }

    const char *src_file = argv[1];
    const char *dest_file = argv[2];

    struct io_uring ring;
    struct io_uring_cqe *cqe;
    struct io_uring_sqe *sqe;
    struct iovec iov;
    int src_fd, dest_fd;
    off_t offset = 0;
    ssize_t ret;

    // Initialize io_uring
    if (io_uring_queue_init(QUEUE_DEPTH, &ring, 0) < 0) {
        handle_error("io_uring_queue_init");
    }

    // Open source and destination files
    src_fd = open(src_file, O_RDONLY);
    if (src_fd < 0) {
        handle_error("open src_file");
    }

    dest_fd = open(dest_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dest_fd < 0) {
        handle_error("open dest_file");
    }

    // Allocate buffer
    char *buf = (char *) malloc(BLOCK_SIZE);
    if (!buf) {
        handle_error("malloc");
    }

    // Set up the iovec structure
    iov.iov_base = buf;
    iov.iov_len = BLOCK_SIZE_DEMO;

    // Submit the read request
    sqe = io_uring_get_sqe(&ring);
    io_uring_prep_readv(sqe, src_fd, &iov, 1, offset);
    io_uring_sqe_set_flags(sqe, 0);
    io_uring_submit(&ring);

    // Wait for the read to complete
    ret = io_uring_wait_cqe(&ring, &cqe);
    if (ret < 0) {
        handle_error("io_uring_wait_cqe");
    }

    // Check for read errors
    if (cqe->res < 0) {
        fprintf(stderr, "Read failed: %s\n", strerror(-cqe->res));
        exit(1);
    }

    // Get the number of bytes read
    size_t bytes_read = cqe->res;
    io_uring_cqe_seen(&ring, cqe); // marked as seen

    // Submit the write request
    sqe = io_uring_get_sqe(&ring);
    io_uring_prep_writev(sqe, dest_fd, &iov, 1, offset);
    io_uring_sqe_set_flags(sqe, 0);
    io_uring_submit(&ring);

    // Wait for the write to complete
    ret = io_uring_wait_cqe(&ring, &cqe);
    if (ret < 0) {
        handle_error("io_uring_wait_cqe");
    }

    // Check for write errors
    if (cqe->res < 0) {
        fprintf(stderr, "Write failed: %s\n", strerror(-cqe->res));
        exit(1);
    }

    // Get the number of bytes written
    size_t bytes_written = cqe->res;
    io_uring_cqe_seen(&ring, cqe);

    // Clean up
    free(buf);
    close(src_fd);
    close(dest_fd);
    io_uring_queue_exit(&ring);

    printf("Read %zu bytes and wrote %zu bytes\n", bytes_read, bytes_written);

    return 0;
}
