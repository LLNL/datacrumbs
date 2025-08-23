#include <assert.h>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <mpi.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <unistd.h>
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
  static const char alphanum[] = "0123456789"
                                 "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                 "abcdefghijklmnopqrstuvwxyz";
  std::string tmp_s;
  tmp_s.reserve(len);

  for (int i = 0; i < len; ++i) {
    tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];
  }

  return tmp_s;
}
int main(int argc, char *argv[]) {
  MPI_Init(&argc, &argv);
  int my_rank, comm_size;
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
  int files = atoi(argv[1]);
  int ops = atoi(argv[2]);
  int ts = atoi(argv[3]);
  std::string dir = std::string(argv[4]);
  int test_flag = atoi(argv[5]);
  int direct_io_flag = atoi(argv[6]);
  std::string data = gen_random(ts);
  char *read_data = (char *)malloc(ts);
  Timer open_timer = Timer();
  Timer write_timer = Timer();
  Timer read_timer = Timer();
  Timer close_timer = Timer();
  int sleep_time = 0;
  for (int file_idx = 0; file_idx < files; ++file_idx) {

    std::string filename = dir + "/file_" + std::to_string(file_idx) + "_" +
                           std::to_string(my_rank) + ".dat";

    open_timer.resumeTime();

    FILE *fh;
    if (test_flag == 0) {
      fh = fopen(filename.c_str(), "w+");
      // fd = open(filename.c_str(), flag, 0777);
    } else if (test_flag == 1) {
      fh = fopen(filename.c_str(), "r");
    } else {
      fh = fopen(filename.c_str(), "w+");
    }
    if (sleep_time > 0) {
      printf("Sleeping for %d\n", sleep_time);
      sleep(sleep_time);
    }
    assert(fh != NULL);
    open_timer.pauseTime();
    for (int op_idx = 0; op_idx < ops; ++op_idx) {
      if (sleep_time > 0) {
        printf("Sleeping for write for %d for step %d of %d\n", sleep_time, op_idx, ops);
        sleep(sleep_time);
      }
      if (test_flag == 0 || test_flag == 2) {
        write_timer.resumeTime();
        assert(fwrite(data.c_str(), ts, 1, fh) == 1);
        write_timer.pauseTime();
      }

      if (test_flag == 2) {
        if (sleep_time > 0) {
            printf("Sleeping for fseek for %d for step %d of %d\n", sleep_time, op_idx, ops);
            sleep(sleep_time);
        }
        fseek(fh, (off_t)op_idx * ts, SEEK_SET);
      }
      if (test_flag == 1 || test_flag == 2) {
        if (sleep_time > 0) {
            printf("Sleeping for read for %d for step %d of %d\n", sleep_time, op_idx, ops);
            sleep(sleep_time);
        }
        read_timer.resumeTime();
        assert(fread(read_data, ts, 1, fh) == 1);
        read_timer.pauseTime();
      }
    }
    if (sleep_time > 0) {
      printf("Sleeping for close for %d\n", sleep_time);
      sleep(sleep_time);
    }
    close_timer.resumeTime();
    fclose(fh);
    close_timer.pauseTime();
  }
  free(read_data);
  double open_time = open_timer.getElapsedTime();
  double total_open_time;
  MPI_Reduce(&open_time, &total_open_time, 1, MPI_DOUBLE, MPI_SUM, 0,
             MPI_COMM_WORLD);
  double close_time = close_timer.getElapsedTime();
  double total_close_time;
  MPI_Reduce(&close_time, &total_close_time, 1, MPI_DOUBLE, MPI_SUM, 0,
             MPI_COMM_WORLD);
  double write_time = write_timer.getElapsedTime();
  double total_write_time;
  MPI_Reduce(&write_time, &total_write_time, 1, MPI_DOUBLE, MPI_SUM, 0,
             MPI_COMM_WORLD);
  double read_time = read_timer.getElapsedTime();
  double total_read_time;
  MPI_Reduce(&read_time, &total_read_time, 1, MPI_DOUBLE, MPI_SUM, 0,
             MPI_COMM_WORLD);
  if (my_rank == 0) {
    printf("%d,%d,%d,%d,%f,%f,%f,%f\n", comm_size, test_flag, ops, ts,
           total_open_time / comm_size, total_close_time / comm_size,
           total_write_time / comm_size, total_read_time / comm_size);
  }
  MPI_Finalize();
  return 0;
}