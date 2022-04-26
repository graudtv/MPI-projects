# Try to build everything available to check if new changes to
# cxxmpi has broken old code

# exit if any command fails
set -e
trap 'echo -- Build failed!!!' EXIT

build_experiments() {
  script_path=$(dirname "$0")
  pushd "$script_path/.." >/dev/null
  # I had to use find with full path because very is no readlink/realpath
  # on OsX to display currently processed path in a pretty way
  for file in $(find "$(pwd)/experiments" -name "Makefile"); do
    dir=$(dirname $file)
    echo "-- Building $dir..."
    make -C $dir clean
    make -C $dir
  done;
  popd >/dev/null
}

run_unit_tests() {
  script_path=$(dirname "$0")
  if ! [ -e "$script_path/cxxmpi/build" ]; then
    mkdir "$script_path/cxxmpi/build"
    cd "$script_path/cxxmpi/build"
    cmake ..
  else
    cd "$script_path/cxxmpi/build"
  fi
  echo "-- Building cxxmpi unit tests"
  make
  ./unit-tests
}

build_experiments
run_unit_tests
echo "-- SUCCESS!"
trap '' EXIT