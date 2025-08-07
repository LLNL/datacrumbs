#include <assert.h>
#include <fcntl.h>
#include <mpi.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

class Timer {
 public:
  Timer() : elapsed_time(0) {}
  void resumeTime() { t1 = std::chrono::high_resolution_clock::now(); }
  double pauseTime() {
    auto t2 = std::chrono::high_resolution_clock::now();
    elapsed_time += std::chrono::duration<double>(t2 - t1).count();
    return elapsed_time;
  }
  double getElapsedTime() { return elapsed_time; }

 private:
  std::chrono::high_resolution_clock::time_point t1;
  double elapsed_time;
};
std::string gen_random(const int len) {
  static const char alphanum[] =
      "0123456789"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz";
  std::string tmp_s;
  tmp_s.reserve(len);

  for (int i = 0; i < len; ++i) {
    tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];
  }

  return tmp_s;
}
int test_open_perm(const char* filename, int flag, int perm) {
  int fd = open(filename, flag, perm);
  if (fd == -1) {
    perror("open");
    return -1;
  }
  return fd;
}
int test_open(const char* filename, int flag) {
  int fd = open(filename, flag, 0777);
  if (fd == -1) {
    perror("open");
    return -1;
  }
  return fd;
}
ssize_t test_read(int fd, void* buf, ssize_t size) {
  ssize_t bytes = read(fd, buf, size);
  if (bytes == -1) {
    fprintf(stderr, "read : %s (errno: %d)\n", strerror(errno), errno);
    return -1;
  }
  return bytes;
}
ssize_t test_write(int fd, const void* buf, ssize_t size) {
  ssize_t bytes = write(fd, buf, size);
  if (bytes == -1) {
    fprintf(stderr, "write on fd:%d of size:%ld failed with %ld: %s (errno: %d)\n", fd, size, bytes,
            strerror(errno), errno);
    return -1;
  }
  return bytes;
}
int test_close(int fd) {
  int ret = close(fd);
  if (ret == -1) {
    perror("close");
    return -1;
  }
  return ret;
}
int test_lseek(int fd, off_t offset, int whence) {
  int ret = lseek(fd, offset, whence);
  if (ret == -1) {
    perror("lseek");
    return -1;
  }
  return ret;
}
int test_fsync(int fd) {
  int ret = fsync(fd);
  if (ret == -1) {
    perror("fsync");
    return -1;
  }
  return ret;
}

int main(int argc, char* argv[]) {
  MPI_Init(&argc, &argv);
  int my_rank, comm_size;
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
  int files = atoi(argv[1]);
  int ops = atoi(argv[2]);
  ssize_t ts = atol(argv[3]);
  std::string dir = std::string(argv[4]);
  int test_flag = atoi(argv[5]);
  int direct_io_flag = atoi(argv[6]);
  int sleep_time = atoi(argv[7]);
  struct stat st;
  if (stat(dir.c_str(), &st) != 0) {
    perror("stat");
    MPI_Abort(MPI_COMM_WORLD, 1);
  }
  size_t alignment = st.st_blksize;
  void* data_aligned = nullptr;
  void* read_data_aligned = nullptr;
  if (posix_memalign(&data_aligned, alignment, ts) != 0) {
    perror("posix_memalign data");
    MPI_Abort(MPI_COMM_WORLD, 1);
  }
  if (posix_memalign(&read_data_aligned, alignment, ts) != 0) {
    perror("posix_memalign read_data");
    MPI_Abort(MPI_COMM_WORLD, 1);
  }
  char* data = (char*)data_aligned;
  char* read_data = (char*)read_data_aligned;
  // for (int i = 0; i < ts; ++i) {
  //   ((char *)data)[i] = 'a' + (i % 26); // Fill with some data
  // }
  Timer open_timer = Timer();
  Timer write_timer = Timer();
  Timer read_timer = Timer();
  Timer close_timer = Timer();
  for (int file_idx = 0; file_idx < files; ++file_idx) {
    std::string filename =
        dir + "/file_" + std::to_string(file_idx) + "_" + std::to_string(my_rank) + ".dat";

    open_timer.resumeTime();

    int fd = -1;
    if (test_flag == 0) {
      int flag = O_WRONLY | O_CREAT | O_TRUNC;
      if (direct_io_flag == 1) {
        flag = O_WRONLY | O_CREAT | O_TRUNC | O_DIRECT;
      }
      fd = test_open_perm(filename.c_str(), flag, 0777);
    } else if (test_flag == 1) {
      int flag = O_RDONLY;
      if (direct_io_flag == 1) {
        flag = O_RDONLY | O_DIRECT;
      }
      fd = test_open(filename.c_str(), flag);
    } else {
      int flag = O_RDWR | O_CREAT | O_TRUNC;
      if (direct_io_flag == 1) {
        flag = O_RDWR | O_CREAT | O_TRUNC | O_DIRECT;
      }
      fd = test_open_perm(filename.c_str(), flag, 0777);
    }

    open_timer.pauseTime();
    if (fd == -1) {
      fprintf(stderr, "Error opening file:%s %s (errno: %d)\n", filename.c_str(), strerror(errno),
              errno);
      assert(fd != -1);
    }
    printf("Opened file: %s with fd: %d, ts: %zd\n", filename.c_str(), fd, ts);
    if (sleep_time > 0) {
      printf("Sleeping for %d\n", sleep_time);
      sleep(sleep_time);
    }
    for (int op_idx = 0; op_idx < ops; ++op_idx) {
      if (test_flag == 0 || test_flag == 2) {
        if (sleep_time > 0) {
          printf("Sleeping for write for %d for step %d of %d\n", sleep_time, op_idx, ops);
          sleep(sleep_time);
        }
        write_timer.resumeTime();
        assert(test_write(fd, data, ts) == ts);
        write_timer.pauseTime();
      }

      if (test_flag == 2) {
        if (sleep_time > 0) {
          printf("Sleeping for fseek for %d for step %d of %d\n", sleep_time, op_idx, ops);
          sleep(sleep_time);
        }
        test_lseek(fd, (off_t)op_idx * ts, SEEK_SET);
      }
      if (test_flag == 1 || test_flag == 2) {
        if (sleep_time > 0) {
          printf("Sleeping for read for %d for step %d of %d\n", sleep_time, op_idx, ops);
          sleep(sleep_time);
        }
        read_timer.resumeTime();
        auto read_bytes = test_read(fd, read_data, ts);
        read_timer.pauseTime();
      }
    }
    if (sleep_time > 0) {
      printf("Sleeping for close for %d\n", sleep_time);
      sleep(sleep_time);
    }
    close_timer.resumeTime();
    test_close(fd);
    close_timer.pauseTime();
  }
  free(read_data);
  double open_time = open_timer.getElapsedTime();
  double total_open_time;
  MPI_Reduce(&open_time, &total_open_time, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
  double close_time = close_timer.getElapsedTime();
  double total_close_time;
  MPI_Reduce(&close_time, &total_close_time, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
  double write_time = write_timer.getElapsedTime();
  double total_write_time;
  MPI_Reduce(&write_time, &total_write_time, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
  double read_time = read_timer.getElapsedTime();
  double total_read_time;
  MPI_Reduce(&read_time, &total_read_time, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
  if (my_rank == 0) {
    printf("%d,%d,%d,%ld,%f,%f,%f,%f\n", comm_size, test_flag, ops, ts, total_open_time / comm_size,
           total_close_time / comm_size, total_write_time / comm_size, total_read_time / comm_size);
  }
  MPI_Finalize();
  return 0;
}