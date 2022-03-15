# Try to build everything available to check if new changes to
# cxxmpi has broken old code

# exit if any command fails
set -e
trap 'echo -- Build failed!!!' EXIT

build_experiments() {
  script_path=$(dirname "$0")
  pushd "$script_path/../experiments" >/dev/null
  
  for folder in *; do
    pushd "$folder" >/dev/null
    echo "-- Building experiments/$folder..."
    make clean
    make
    popd >/dev/null
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
pwd
echo "-- SUCCESS!"
trap '' EXIT