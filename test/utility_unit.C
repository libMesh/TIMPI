#include <timpi/timpi.h>

#include <timpi/timpi_version.h>

#define TIMPI_UNIT_ASSERT(expr) \
  if (!(expr)) \
    timpi_error();

using namespace TIMPI;

Communicator *TestCommWorld;

  void testVersionString()
  {
    std::string build_string = timpi_version_string();
    TIMPI_UNIT_ASSERT(build_string.find("Version = 1.8.5") != std::string::npos);
    TIMPI_UNIT_ASSERT(build_string.find("Build Date") != std::string::npos);
    TIMPI_UNIT_ASSERT(build_string.find("C++ Config") != std::string::npos);
    TIMPI_UNIT_ASSERT(build_string.find("Using find correctly") == std::string::npos);
  }

  void testVersionNumber()
  {
    int version = get_timpi_version();
    TIMPI_UNIT_ASSERT(version == 10805);
  }


int main(int argc, const char * const * argv)
{
  TIMPI::TIMPIInit init(argc, argv);
  TestCommWorld = &init.comm();

  testVersionString();
  testVersionNumber();

  return 0;
}
