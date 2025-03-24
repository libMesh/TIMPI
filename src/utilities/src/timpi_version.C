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

#include "timpi/timpi_version.h"

#include <iostream>
#include <sstream>

namespace TIMPI
{

  void timpi_version_stdout()
  {
    std::cout << timpi_version_string() << std::flush;
  }

  std::string timpi_version_string()
  {
    std::ostringstream oss;

    oss << "--------------------------------------------------------" << std::endl;
    oss << "TIMPI Package: Version = " << TIMPI_LIB_VERSION;
    oss << " (" << get_timpi_version() << ")" << std::endl << std::endl;

    oss << TIMPI_LIB_RELEASE << std::endl << std::endl;

    oss << "Build Date   = " << TIMPI_BUILD_DATE     << std::endl;
    oss << "Build Host   = " << TIMPI_BUILD_HOST     << std::endl;
    oss << "Build User   = " << TIMPI_BUILD_USER     << std::endl;
    oss << "Build Arch   = " << TIMPI_BUILD_ARCH     << std::endl;
    oss << "Build Rev    = " << TIMPI_BUILD_VERSION  << std::endl << std::endl;

    oss << "C++ Config   = " << TIMPI_CXX << " " << TIMPI_CXXFLAGS << std::endl;
    oss << "--------------------------------------------------------" << std::endl;

    return oss.str();
  }

  int get_timpi_version()
  {
    /* Note: return format follows the versioning convention xx.yy.zz where

       xx = major version number
       yy = minor version number
       zz = micro version number

       For example:
       v.   0.23  -> 002300 = 2300
       v   0.23.1 -> 002301 = 2301
       v. 10.23.2 -> 102302         */

    int major_version = 0;
    int minor_version = 0;
    int micro_version = 0;

#ifdef TIMPI_MAJOR_VERSION
    major_version = TIMPI_MAJOR_VERSION;
#endif

#ifdef TIMPI_MINOR_VERSION
    minor_version = TIMPI_MINOR_VERSION;
#endif

#ifdef TIMPI_MICRO_VERSION
    micro_version = TIMPI_MICRO_VERSION;
#endif

    return major_version*10000 + minor_version*100 + micro_version;
  }

} // end namespace TIMPI
