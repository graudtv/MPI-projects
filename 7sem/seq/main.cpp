#include <omp.h>
#include <cstdio>
#include <cstdlib>
#include <vector>

int main(int argc, char *argv[]) {
  auto max_threads = omp_get_max_threads();
  std::vector<omp_lock_t> locks(max_threads);

  for (auto &&lock : locks)
    omp_init_lock(&lock);
  for (size_t i = 1; i < locks.size(); ++i)
    omp_set_lock(&locks[i]);

#pragma omp parallel
  {
    auto id = omp_get_thread_num();
    omp_set_lock(&locks[id]);
    printf("thread %d\n", id);
    if (id != locks.size())
      omp_unset_lock(&locks[id + 1]);
  }
  return 0;
}
