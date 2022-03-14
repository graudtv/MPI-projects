/* This is the top header of cxxmpi which includes all others.
 *
 * Note. Some includes here are redundant, but I think is's better
 * to catch dependency errors here than inside file hierarchy. It
 * also explicitly establishes the main cxxmpi components,
 * which may be used independently if desired
 */

#pragma once

#include <mpi.h>
#include "Shared/misc.hpp"
#include "P2P/BlockingMessages.hpp"
#include "Collective/CollectiveMessages.hpp"