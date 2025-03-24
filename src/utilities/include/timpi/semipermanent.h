// The TIMPI Message-Passing Parallelism Library.
// Copyright (C) 2002-2025 Benjamin S. Kirk, John W. Peterson, Roy H. Stogner

// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA



#ifndef TIMPI_SEMIPERMANENT_H
#define TIMPI_SEMIPERMANENT_H


// Local includes
#include "timpi/timpi_config.h"

// C/C++ includes

#include <memory>
#include <vector>

namespace TIMPI
{

// Forward declare friend
class TIMPIInit;

/**
 * The \p SemiPermanent "class" is basically just a place for a
 * destructor vtable.  Derive from it and pass a unique_ptr to your
 * derived object to SemiPermanent::add() whenever you have
 * something that ought to be *almost* permanent: that should be
 * cleaned up eventually to avoid resource leaks, but that should not
 * be cleaned up until the last TIMPIInit object exits, just before
 * the MPI_Finalize call if TIMPI initialized MPI.
 */

class SemiPermanent
{
public:
  SemiPermanent() = default;
  virtual ~SemiPermanent() = default;

  /*
   * Transfer ownership of a pointer to some "SemiPermanent" objects,
   * to be destroyed (and thereby do any necessary cleanup work)
   * whenever the last TIMPIInit is destroyed, before MPI is
   * finalized.
   */
  static void add(std::unique_ptr<SemiPermanent> obj);

  // Class to hold a reference to the SemiPermanent objects; they will
  // only be destructed when the last Ref is.
  struct Ref {
    Ref() { _ref_count++; }
    ~Ref();
  };

private:

  // Mechanisms to avoid leaks after every TIMPIInit has been
  // destroyed
  static int _ref_count;
  static std::vector<std::unique_ptr<SemiPermanent>> _stuff_to_clean;
};


} // namespace TIMPI

#endif // TIMPI_SEMIPERMANENT_H
