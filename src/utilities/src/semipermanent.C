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


// Local includes
#include "timpi/semipermanent.h"

#include "timpi/timpi_assert.h"

namespace TIMPI
{

int SemiPermanent::_ref_count = 0;

std::vector<std::unique_ptr<SemiPermanent>> SemiPermanent::_stuff_to_clean;


SemiPermanent::Ref::~Ref()
{
  _ref_count--;

  // Last one to leave turns out the lights
  if (_ref_count <= 0)
    _stuff_to_clean.clear();
}


void SemiPermanent::add(std::unique_ptr<SemiPermanent> obj)
{
  timpi_assert_greater(_ref_count, 0);

  _stuff_to_clean.push_back(std::move(obj));
}

}


