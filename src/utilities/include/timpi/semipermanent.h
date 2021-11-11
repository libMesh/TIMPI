// The TIMPI Message-Passing Parallelism Library.
// Copyright (C) 2002-2019 Benjamin S. Kirk, John W. Peterson, Roy H. Stogner

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


namespace TIMPI
{

/**
 * The \p SemiPermanent "class" is basically just a place for a
 * destructor vtable.  Derive from it and pass a unique_ptr to your
 * derived object to TIMPIInit::add_semipermanent() whenever you have
 * something that ought to be *almost* permanent: that should be
 * cleaned up eventually to avoid resource leaks, but that should not
 * be cleaned up until the last TIMPIInit object exits, just before
 * the MPI_Finalize call if TIMPI initialized MPI.
 */

struct SemiPermanent
{
  SemiPermanent() = default;
  virtual ~SemiPermanent() = default;
};


} // namespace TIMPI

#endif // TIMPI_SEMIPERMANENT_H
