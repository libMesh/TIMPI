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
#include "timpi/timpi_init.h"

#include "timpi/semipermanent.h"

// TIMPI includes
#include "timpi/communicator.h"
#include "timpi/timpi_assert.h"


#ifdef TIMPI_HAVE_MPI
void TIMPI_MPI_Handler (MPI_Comm *, int *, ...)
{
  timpi_not_implemented();
}
#endif


namespace TIMPI
{

#ifdef TIMPI_HAVE_MPI
TIMPIInit::TIMPIInit (int argc, const char * const * argv,
                            int mpi_thread_requested,
                            bool handle_mpi_errors,
                            MPI_Comm COMM_WORLD_IN) :
  i_initialized_mpi(false),
  err_handler_set(false)
{
  // Check whether the calling program has already initialized
  // MPI, and avoid duplicate Init/Finalize
  int flag;
  timpi_call_mpi(MPI_Initialized (&flag));

  if (!flag)
    {
      int mpi_thread_provided;

      timpi_call_mpi
        (MPI_Init_thread (&argc, const_cast<char ***>(&argv),
                          mpi_thread_requested, &mpi_thread_provided));

      if (mpi_thread_provided < mpi_thread_requested)
        {
          // Ideally, if an MPI stack tells us it's unsafe for us
          // to use threads, we should scream and die or at least
          // disable threads.
          //
          // In practice, we've encountered one MPI stack (an mvapich2
          // configuration) that returned MPI_THREAD_SINGLE as a
          // proper warning, two stacks that handle
          // MPI_THREAD_FUNNELED properly, and two current stacks plus
          // a couple old stacks that return MPI_THREAD_SINGLE but
          // support threaded runs anyway, so we just emit a warning.
          //
          std::string thread_type;
          switch (mpi_thread_requested)
            {
            case 0:
              thread_type = "MPI_THREAD_SINGLE";
              break;
            case 1:
              thread_type = "MPI_THREAD_FUNNELED";
              break;
            case 2:
              thread_type = "MPI_THREAD_SERIALIZED";
              break;
            case 3:
              thread_type = "MPI_THREAD_MULTIPLE";
              break;
            default:
              timpi_error_msg("Unsupported mpi thread requested '" << mpi_thread_requested << "'");
            }

          timpi_warning("Warning: MPI failed to guarantee " << thread_type << "\n"
                           << "for a threaded run.\n"
                           << std::endl);
        }
      this->i_initialized_mpi = true;
    }

  // Duplicate the input communicator for internal use
  // And get a Communicator copy too, to use
  // as a default for that API
  this->_comm = std::make_unique<Communicator>(COMM_WORLD_IN);

  // Let SemiPermanent know we need its objects for a while
  this->_ref = std::make_unique<SemiPermanent::Ref>();

  // Set up an MPI error handler if requested.  This helps us get
  // into a debugger with a proper stack when an MPI error occurs.
  if (handle_mpi_errors)
    {
      timpi_call_mpi
        (MPI_Comm_create_errhandler(TIMPI_MPI_Handler, &my_errhandler));
      timpi_call_mpi
        (MPI_Comm_set_errhandler(COMM_WORLD_IN, my_errhandler));
      timpi_call_mpi
        (MPI_Comm_set_errhandler(MPI_COMM_WORLD, my_errhandler));
      err_handler_set = true;
    }
}
#else
TIMPIInit::TIMPIInit (int /* argc */, const char * const * /* argv */,
                      int /* mpi_thread_requested */,
                      bool /* handle_mpi_errors */)
{
  this->_comm = std::make_unique<Communicator>(); // So comm() doesn't dereference null
  this->_ref = std::make_unique<SemiPermanent::Ref>();
}
#endif



TIMPIInit::~TIMPIInit()
{
  // Every processor had better be ready to exit at the same time.
  // This would be a timpi_parallel_only() function, except that
  // timpi_parallel_only() uses timpi_assert() which throws an
  // exception which causes compilers to scream about exceptions
  // inside destructors.

  // Even if we're not doing parallel_only debugging, we don't want
  // one processor to try to exit until all others are done working.
  this->comm().barrier();

  // Trigger any SemiPermanent cleanup before potentially finalizing MPI
  _ref.reset();

#ifdef TIMPI_HAVE_MPI
  if (err_handler_set)
    {
      unsigned int error_code =
        MPI_Errhandler_free(&my_errhandler);
      if (error_code != MPI_SUCCESS)
        {
          std::cerr <<
            "Failure when freeing MPI_Errhandler! Continuing..." <<
            std::endl;
        }
    }

  this->_comm.reset();

  if (this->i_initialized_mpi)
    {
      // We can't just timpi_assert here because destructor,
      // but we ought to report any errors
      int error_code = MPI_Finalize();
      if (error_code != MPI_SUCCESS)
        {
          char error_string[MPI_MAX_ERROR_STRING+1];
          int error_string_len;
          MPI_Error_string(error_code, error_string,
                           &error_string_len);
          std::cerr << "Failure from MPI_Finalize():\n"
                    << error_string << std::endl;
        }
    }
#else
  this->_comm.reset();
#endif
}


} // namespace TIMPI
