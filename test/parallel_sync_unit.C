#include <timpi/parallel_sync.h>
#include <timpi/timpi_init.h>

#include <algorithm>
#include <regex>

#define TIMPI_UNIT_ASSERT(expr) \
  if (!(expr)) \
    timpi_error();

using namespace TIMPI;

Communicator *TestCommWorld;

  // Data to send/recieve with each processor rank.  For this test,
  // processor p will send to destination d the integer d, in a vector
  // with sqrt(c)+1 copies, iff c := |p-d| is a square number.
  void fill_scalar_data
    (std::map<processor_id_type, std::vector<unsigned int>> & data,
     int M)
  {
    const int rank = TestCommWorld->rank();
    for (int d=0; d != M; ++d)
      {
        int diffsize = std::abs(d-rank);
        int diffsqrt = std::sqrt(diffsize);
        if (diffsqrt*diffsqrt == diffsize)
          for (int i=-1; i != diffsqrt; ++i)
            data[d].push_back(d);
      }
  }


  // Multimap data to send/recieve with each processor rank.  For this
  // test, processor p will send to destination d the integer d, in a
  // vector with sqrt(c)+1 copies followed by a vector with 1 copy,
  // iff c := |p-d| is a square number.
  void fill_scalar_data
    (std::multimap<processor_id_type, std::vector<unsigned int>> & data,
     int M)
  {
    const int rank = TestCommWorld->rank();
    for (int d=0; d != M; ++d)
      {
        int diffsize = std::abs(d-rank);
        int diffsqrt = std::sqrt(diffsize);
        if (diffsqrt*diffsqrt == diffsize)
          {
            std::vector<unsigned int> v;
            for (int i=-1; i != diffsqrt; ++i)
              v.push_back(d);
            data.emplace(d, v);
            v.resize(1, d);
            data.emplace(d, v);
          }
      }
  }


  // Data to send/recieve with each processor rank.  For this test,
  // processor p will send to destination d the integer d, in two
  // subvectors with sqrt(c) and 1 copies, iff c := |p-d| is a square
  // number.
  void fill_vector_data
    (std::map<processor_id_type, std::vector<std::vector<unsigned int>>> & data,
     int M)
  {
    const int rank = TestCommWorld->rank();
    for (int d=0; d != M; ++d)
      {
        int diffsize = std::abs(d-rank);
        int diffsqrt = std::sqrt(diffsize);
        if (diffsqrt*diffsqrt == diffsize)
          {
            data[d].resize(2);
            for (int i=-1; i != diffsqrt; ++i)
              data[d][0].push_back(d);
            data[d][1].push_back(d);
          }
      }
  }



  // Multimap data to send/recieve with each processor rank.  For this
  // test, processor p will send to destination d the integer d, in
  // two subvectors with sqrt(c) and 1 copies, followed by a vector
  // with 1 copy, iff c := |p-d| is a square number.
  void fill_vector_data
    (std::multimap<processor_id_type, std::vector<std::vector<unsigned int>>> & data,
     int M)
  {
    const int rank = TestCommWorld->rank();
    for (int d=0; d != M; ++d)
      {
        int diffsize = std::abs(d-rank);
        int diffsqrt = std::sqrt(diffsize);
        if (diffsqrt*diffsqrt == diffsize)
          {
            std::vector<std::vector<unsigned int>> vv(2);
            for (int i=-1; i != diffsqrt; ++i)
              vv[0].push_back(d);
            vv[1].push_back(d);
            data.emplace(d, vv);
            vv.resize(1);
            vv[0].resize(1);
            data.emplace(d, vv);
          }
      }
  }


  void testPushImpl(int M)
  {
    const int size = TestCommWorld->size(),
              rank = TestCommWorld->rank();

    std::map<processor_id_type, std::vector<unsigned int> > data, received_data;

    fill_scalar_data(data, M);

    auto collect_data =
      [&received_data]
      (processor_id_type pid,
       const typename std::vector<unsigned int> & vec_received)
      {
        auto & vec = received_data[pid];
        vec.insert(vec.end(), vec_received.begin(), vec_received.end());
      };

    TIMPI::push_parallel_vector_data(*TestCommWorld, data, collect_data);

    // Test the received results, for each processor id p we're in
    // charge of.
    std::vector<std::size_t> checked_sizes(size, 0);
    for (int p=rank; p < M; p += size)
      for (int srcp=0; srcp != size; ++srcp)
        {
          int diffsize = std::abs(srcp-p);
          int diffsqrt = std::sqrt(diffsize);
          if (diffsqrt*diffsqrt != diffsize)
            {
              if (received_data.count(srcp))
                {
                  const std::vector<unsigned int> & datum = received_data[srcp];
                  TIMPI_UNIT_ASSERT(std::count(datum.begin(), datum.end(), p) == std::ptrdiff_t(0));
                }
              continue;
            }

          TIMPI_UNIT_ASSERT(received_data.count(srcp) == std::size_t(1));
          const std::vector<unsigned int> & datum = received_data[srcp];
          TIMPI_UNIT_ASSERT(std::count(datum.begin(), datum.end(), p) == std::ptrdiff_t(diffsqrt+1));
          checked_sizes[srcp] += diffsqrt+1;
        }

    for (int srcp=0; srcp != size; ++srcp)
      TIMPI_UNIT_ASSERT(checked_sizes[srcp] == received_data[srcp].size());
  }


  void testPush()
  {
    testPushImpl(TestCommWorld->size());
  }


  void testPushOversized()
  {
    testPushImpl((TestCommWorld->size() + 4) * 2);
  }


  void testPushMove()
  {
    const int size = TestCommWorld->size(),
              rank = TestCommWorld->rank();

    std::map<processor_id_type, std::vector<unsigned int>> data, received_data;

    const int M = TestCommWorld->size();
    fill_scalar_data(data, M);

    auto collect_data =
      [&received_data]
      (processor_id_type pid,
       typename std::vector<unsigned int> && vec_received)
      {
        auto & vec = received_data[pid];
        for (auto & val : vec_received)
          vec.emplace_back(std::move(val));
      };

    TIMPI::push_parallel_vector_data(*TestCommWorld, std::move(data), collect_data);

    // Test the received results, for each processor id p we're in
    // charge of.
    std::vector<std::size_t> checked_sizes(size, 0);
    for (int p=rank; p < M; p += size)
      for (int srcp=0; srcp != size; ++srcp)
        {
          int diffsize = std::abs(srcp-p);
          int diffsqrt = std::sqrt(diffsize);
          if (diffsqrt*diffsqrt != diffsize)
            {
              if (received_data.count(srcp))
                {
                  const std::vector<unsigned int> & datum = received_data[srcp];
                  TIMPI_UNIT_ASSERT(std::count(datum.begin(), datum.end(), p) == std::ptrdiff_t(0));
                }
              continue;
            }

          TIMPI_UNIT_ASSERT(received_data.count(srcp) == std::size_t(1));
          const std::vector<unsigned int> & datum = received_data[srcp];
          TIMPI_UNIT_ASSERT(std::count(datum.begin(), datum.end(), p) == std::ptrdiff_t(diffsqrt+1));
          checked_sizes[srcp] += diffsqrt+1;
        }

    for (int srcp=0; srcp != size; ++srcp)
      TIMPI_UNIT_ASSERT(checked_sizes[srcp] == received_data[srcp].size());
  }


  void testPullImpl(int M)
  {
    // Oversized pulls are well-defined with NBX and ALLTOALL_COUNTS
    // because of C++11's guarantees regarding preservation of insert
    // ordering in multimaps, combined with MPI's guarantees about
    // non-overtaking ... but we do receives in a different order with
    // SENDRECEIVE mode, so let's skip oversized test there.
    if (TestCommWorld->sync_type() == Communicator::SENDRECEIVE &&
        M > int(TestCommWorld->size()))
      return;

    std::map<processor_id_type, std::vector<unsigned int> > data, received_data;

    fill_scalar_data(data, M);

    auto compose_replies =
      []
      (processor_id_type /* pid */,
       const std::vector<unsigned int> & query,
       std::vector<unsigned int> & response)
      {
        const std::size_t query_size = query.size();
        response.resize(query_size);
        for (unsigned int i=0; i != query_size; ++i)
          response[i] = query[i]*query[i];
      };


    auto collect_replies =
      [&received_data]
      (processor_id_type pid,
       const std::vector<unsigned int> & query,
       const std::vector<unsigned int> & response)
      {
        const std::size_t query_size = query.size();
        TIMPI_UNIT_ASSERT(query_size == response.size());
        for (unsigned int i=0; i != query_size; ++i)
          {
            TIMPI_UNIT_ASSERT(query[i]*query[i] == response[i]);
          }
        received_data[pid] = response;
      };

    // Do the pull
    unsigned int * ex = nullptr;
    TIMPI::pull_parallel_vector_data
      (*TestCommWorld, data, compose_replies, collect_replies, ex);

    // Test the received results, for each query we sent.
    for (int p=0; p != M; ++p)
      {
        TIMPI_UNIT_ASSERT(data[p].size() == received_data[p].size());
        for (std::size_t i = 0; i != data[p].size(); ++i)
          TIMPI_UNIT_ASSERT(data[p][i]*data[p][i] == received_data[p][i]);
      }
  }


  void testPull()
  {
    testPullImpl(TestCommWorld->size());
  }


  void testPullOversized()
  {
    testPullImpl((TestCommWorld->size() + 4) * 2);
  }


  void testPushVecVecImpl(int M)
  {
    const int size = TestCommWorld->size(),
              rank = TestCommWorld->rank();

    std::map<processor_id_type, std::vector<std::vector<unsigned int>>> data;
    std::map<processor_id_type, std::vector<unsigned int>> received_data;

    fill_vector_data(data, M);

    auto collect_data =
      [&received_data]
      (processor_id_type pid,
       const typename std::vector<std::vector<unsigned int>> & vecvec_received)
      {
        TIMPI_UNIT_ASSERT(vecvec_received.size() == std::size_t(2));
        TIMPI_UNIT_ASSERT(vecvec_received[1].size() == std::size_t(1));
        TIMPI_UNIT_ASSERT(vecvec_received[0][0] == vecvec_received[1][0]);
        auto & vec = received_data[pid];
        vec.insert(vec.end(), vecvec_received[0].begin(), vecvec_received[0].end());
      };

    TIMPI::push_parallel_vector_data(*TestCommWorld, data, collect_data);

    // Test the received results, for each processor id p we're in
    // charge of.
    std::vector<std::size_t> checked_sizes(size, 0);
    for (int p=rank; p < M; p += size)
      for (int srcp=0; srcp != size; ++srcp)
        {
          int diffsize = std::abs(srcp-p);
          int diffsqrt = std::sqrt(diffsize);
          if (diffsqrt*diffsqrt != diffsize)
            {
              if (received_data.count(srcp))
                {
                  const std::vector<unsigned int> & datum = received_data[srcp];
                  TIMPI_UNIT_ASSERT(std::count(datum.begin(), datum.end(), p) == std::ptrdiff_t(0));
                }
              continue;
            }

          TIMPI_UNIT_ASSERT(received_data.count(srcp) == std::size_t(1));
          const std::vector<unsigned int> & datum = received_data[srcp];
          TIMPI_UNIT_ASSERT(std::count(datum.begin(), datum.end(), p) == std::ptrdiff_t(diffsqrt+1));
          checked_sizes[srcp] += diffsqrt+1;
        }

    for (int srcp=0; srcp != size; ++srcp)
      TIMPI_UNIT_ASSERT(checked_sizes[srcp] == received_data[srcp].size());
  }


  void testPushVecVec()
  {
    testPushVecVecImpl(TestCommWorld->size());
  }


  void testPushVecVecOversized()
  {
    testPushVecVecImpl((TestCommWorld->size() + 4) * 2);
  }


  void testPullVecVecImpl(int M)
  {
    // Oversized pulls are well-defined with NBX and ALLTOALL_COUNTS
    // because of C++11's guarantees regarding preservation of insert
    // ordering in multimaps, combined with MPI's guarantees about
    // non-overtaking ... but we do receives in a different order with
    // SENDRECEIVE mode, so let's skip oversized test there.
    if (TestCommWorld->sync_type() == Communicator::SENDRECEIVE &&
        M > int(TestCommWorld->size()))
      return;

    std::map<processor_id_type, std::vector<std::vector<unsigned int>>> data;
    std::map<processor_id_type, std::vector<std::vector<unsigned int>>> received_data;

    fill_vector_data(data, M);

    auto compose_replies =
      []
      (processor_id_type /* pid */,
       const std::vector<std::vector<unsigned int>> & query,
       std::vector<std::vector<unsigned int>> & response)
      {
        const std::size_t query_size = query.size();
        response.resize(query_size);
        for (unsigned int i=0; i != query_size; ++i)
          {
            const std::size_t query_i_size = query[i].size();
            response[i].resize(query_i_size);
            for (unsigned int j=0; j != query_i_size; ++j)
            response[i][j] = query[i][j]*query[i][j];
          }
      };


    auto collect_replies =
      [&received_data]
      (processor_id_type pid,
       const std::vector<std::vector<unsigned int>> & query,
       const std::vector<std::vector<unsigned int>> & response)
      {
        const std::size_t query_size = query.size();
        TIMPI_UNIT_ASSERT(query_size == response.size());
        for (unsigned int i=0; i != query_size; ++i)
          {
            const std::size_t query_i_size = query[i].size();
            TIMPI_UNIT_ASSERT(query_i_size == response[i].size());
            for (unsigned int j=0; j != query_i_size; ++j)
              TIMPI_UNIT_ASSERT(query[i][j]*query[i][j] == response[i][j]);
          }
        auto & vec = received_data[pid];
        vec.emplace_back(response[0].begin(), response[0].end());
        TIMPI_UNIT_ASSERT(response[1].size() == std::size_t(1));
        TIMPI_UNIT_ASSERT(response[1][0] == response[0][0]);
        vec.emplace_back(response[1].begin(), response[1].end());
      };

    // Do the pull
    std::vector<unsigned int> * ex = nullptr;
    TIMPI::pull_parallel_vector_data
      (*TestCommWorld, data, compose_replies, collect_replies, ex);

    // Test the received results, for each query we sent.
    for (int p=0; p != M; ++p)
      {
        TIMPI_UNIT_ASSERT(data[p].size() == received_data[p].size());
        for (std::size_t i = 0; i != data[p].size(); ++i)
          for (std::size_t j = 0; j != data[p][i].size(); ++j)
            TIMPI_UNIT_ASSERT(data[p][i][j]*data[p][i][j] == received_data[p][i][j]);
      }
  }


  void testPullVecVec()
  {
    testPullVecVecImpl(TestCommWorld->size());
  }


  void testPullVecVecOversized()
  {
    testPushVecVecImpl((TestCommWorld->size() + 4) * 2);
  }


  void testPushMultimapImpl(int M)
  {
    const int size = TestCommWorld->size(),
              rank = TestCommWorld->rank();

    // This is going to be well-defined with NBX and ALLTOALL_COUNTS
    // because of C++11's guarantees regarding preservation of insert
    // ordering in multimaps, combined with MPI's guarantees about
    // non-overtaking ... but we do receives in a different order with
    // SENDRECEIVE mode, so let's skip this test there.
    if (TestCommWorld->sync_type() == Communicator::SENDRECEIVE)
      return;

    std::multimap<processor_id_type, std::vector<unsigned int> > data, received_data;

    fill_scalar_data(data, M);

    auto collect_data =
      [&received_data]
      (processor_id_type pid,
       const typename std::vector<unsigned int> & vec_received)
      {
        received_data.emplace(pid, vec_received);
      };

    TIMPI::push_parallel_vector_data(*TestCommWorld, data, collect_data);

    // Test the received results, for each processor id p we're in
    // charge of.
    std::vector<std::size_t> checked_sizes(size, 0);
    for (int p=rank; p < M; p += size)
      for (int srcp=0; srcp != size; ++srcp)
        {
          int diffsize = std::abs(srcp-p);
          int diffsqrt = std::sqrt(diffsize);
          auto rng = received_data.equal_range(srcp);
          if (diffsqrt*diffsqrt != diffsize)
            {
              for (auto pv_it = rng.first; pv_it != rng.second; ++pv_it)
                {
                  TIMPI_UNIT_ASSERT(std::count(pv_it->second.begin(), pv_it->second.end(), p) == std::ptrdiff_t(0));
                }
              continue;
            }

          TIMPI_UNIT_ASSERT(rng.first != rng.second);
          for (auto pv_it = rng.first; pv_it != rng.second; ++pv_it)
            {
              std::ptrdiff_t cnt = std::count(pv_it->second.begin(), pv_it->second.end(), p);
              if (cnt)
                {
                  TIMPI_UNIT_ASSERT(cnt == std::ptrdiff_t(diffsqrt+1));
                  auto pv_it2 = pv_it; ++pv_it2;
                  TIMPI_UNIT_ASSERT(pv_it2 != rng.second);
                  std::ptrdiff_t cnt2 = std::count(pv_it2->second.begin(), pv_it2->second.end(), p);
                  TIMPI_UNIT_ASSERT(cnt2 == std::ptrdiff_t(1));
                  checked_sizes[srcp] += cnt + cnt2;
                  break;
                }
            }
        }

    for (int srcp=0; srcp != size; ++srcp)
      {
        std::size_t total_size = 0;
        auto rng = received_data.equal_range(srcp);
        for (auto pv_it = rng.first; pv_it != rng.second; ++pv_it)
          total_size += pv_it->second.size();
        TIMPI_UNIT_ASSERT(checked_sizes[srcp] == total_size);
      }
  }


  void testPushMultimap()
  {
    testPushMultimapImpl(TestCommWorld->size());
  }


  void testPushMultimapOversized()
  {
    testPushMultimapImpl((TestCommWorld->size() + 4) * 2);
  }


  void testPushMultimapVecVecImpl(int M)
  {
    const int size = TestCommWorld->size(),
              rank = TestCommWorld->rank();

    // This is going to be well-defined with NBX and ALLTOALL_COUNTS
    // because of C++11's guarantees regarding preservation of insert
    // ordering in multimaps, combined with MPI's guarantees about
    // non-overtaking ... but we do receives in a different order with
    // SENDRECEIVE mode, so let's skip this test there.
    if (TestCommWorld->sync_type() == Communicator::SENDRECEIVE)
      return;

    std::multimap<processor_id_type, std::vector<std::vector<unsigned int>>> data, received_data;

    fill_vector_data(data, M);

    auto collect_data =
      [&received_data]
      (processor_id_type pid,
       const typename std::vector<std::vector<unsigned int>> & vecvec_received)
      {
        received_data.emplace(pid, vecvec_received);
      };

    TIMPI::push_parallel_vector_data(*TestCommWorld, data, collect_data);

    // Test the received results, for each processor id p we're in
    // charge of.
    std::vector<std::size_t> checked_sizes(size, 0);
    for (int p=rank; p < M; p += size)
      for (int srcp=0; srcp != size; ++srcp)
        {
          int diffsize = std::abs(srcp-p);
          int diffsqrt = std::sqrt(diffsize);
          auto rng = received_data.equal_range(srcp);
          if (diffsqrt*diffsqrt != diffsize)
            {
              for (auto pvv = rng.first; pvv != rng.second; ++pvv)
                {
                  for (auto & v : pvv->second)
                  TIMPI_UNIT_ASSERT(std::count(v.begin(), v.end(), p) == std::ptrdiff_t(0));
                }
              continue;
            }

          TIMPI_UNIT_ASSERT(rng.first != rng.second);
          for (auto pvv_it = rng.first; pvv_it != rng.second; ++pvv_it)
            {
              if(pvv_it->second.size() != std::size_t(2))
                timpi_error();
              TIMPI_UNIT_ASSERT(pvv_it->second.size() == std::size_t(2));
              std::ptrdiff_t cnt = std::count(pvv_it->second[0].begin(), pvv_it->second[0].end(), p);
              if (cnt)
                {
                  TIMPI_UNIT_ASSERT(cnt == std::ptrdiff_t(diffsqrt+1));
                  std::ptrdiff_t cnt2 = std::count(pvv_it->second[1].begin(), pvv_it->second[1].end(), p);
                  TIMPI_UNIT_ASSERT(cnt2 == std::ptrdiff_t(1));
                  auto pvv_it2 = pvv_it; ++pvv_it2;
                  TIMPI_UNIT_ASSERT(pvv_it2 != rng.second);
                  TIMPI_UNIT_ASSERT(pvv_it2->second.size() == std::size_t(1));
                  std::ptrdiff_t cnt3 = std::count(pvv_it2->second[0].begin(), pvv_it2->second[0].end(), p);
                  TIMPI_UNIT_ASSERT(cnt3 == std::ptrdiff_t(1));
                  checked_sizes[srcp] += cnt + cnt2 + cnt3;
                  break;
                }
              ++pvv_it;
              timpi_assert(pvv_it != rng.second);
            }
        }

    for (int srcp=0; srcp != size; ++srcp)
      {
        std::size_t total_size = 0;
        auto rng = received_data.equal_range(srcp);
        for (auto pvv = rng.first; pvv != rng.second; ++pvv)
          for (auto & v : pvv->second)
            total_size += v.size();
        TIMPI_UNIT_ASSERT(checked_sizes[srcp] == total_size);
      }
  }


  void testPushMultimapVecVec()
  {
    testPushMultimapVecVecImpl(TestCommWorld->size());
  }


  void testPushMultimapVecVecOversized()
  {
    testPushMultimapVecVecImpl((TestCommWorld->size() + 4) * 2);
  }


  // We want to not freak out if we see an empty send in an opt mode
  // sync, but we do want to freak out when debugging.
  void testEmptyEntry()
  {
    const int size = TestCommWorld->size(),
              rank = TestCommWorld->rank();
    const int M=size;

    std::map<processor_id_type, std::vector<unsigned int> > data, received_data;

    fill_scalar_data(data, M);

    // Give some processors empty sends
    for (int i=0; i != M; ++i)
      if (!(rank % 3))
        data[i];

    auto collect_data =
      [&received_data]
      (processor_id_type pid,
       const typename std::vector<unsigned int> & vec_received)
      {
        // Even if we send them we shouldn't get them
        TIMPI_UNIT_ASSERT (!vec_received.empty())

        auto & vec = received_data[pid];
        vec.insert(vec.end(), vec_received.begin(), vec_received.end());
      };

#ifndef NDEBUG
    bool caught_exception = false;
    try {
#endif
    TIMPI::push_parallel_vector_data(*TestCommWorld, data, collect_data);
#ifndef NDEBUG
    }
    catch (std::logic_error & e) {
      caught_exception = true;
      std::regex msg_regex("empty data");
      TIMPI_UNIT_ASSERT(std::regex_search(e.what(), msg_regex));
    }

    // We didn't leave room for empty sends in the 2 processor case
    if (M > 2)
      TIMPI_UNIT_ASSERT(caught_exception);
#endif

    // Test the received results, for each processor id p we're in
    // charge of.
    std::vector<std::size_t> checked_sizes(size, 0);
    for (int p=rank; p < M; p += size)
      for (int srcp=0; srcp != size; ++srcp)
        {
          int diffsize = std::abs(srcp-p);
          int diffsqrt = std::sqrt(diffsize);
          if (diffsqrt*diffsqrt != diffsize)
            {
              if (received_data.count(srcp))
                {
                  const std::vector<unsigned int> & datum = received_data[srcp];
                  TIMPI_UNIT_ASSERT(std::count(datum.begin(), datum.end(), p) == std::ptrdiff_t(0));
                }
              continue;
            }

          TIMPI_UNIT_ASSERT(received_data.count(srcp) == std::size_t(1));
          const std::vector<unsigned int> & datum = received_data[srcp];
          TIMPI_UNIT_ASSERT(std::count(datum.begin(), datum.end(), p) == std::ptrdiff_t(diffsqrt+1));
          checked_sizes[srcp] += diffsqrt+1;
        }

    for (int srcp=0; srcp != size; ++srcp)
      TIMPI_UNIT_ASSERT(checked_sizes[srcp] == received_data[srcp].size());
  }


  void testStringSyncType()
  {
    Communicator comm;
    comm.duplicate(*TestCommWorld);

    comm.sync_type("nbx");
    TIMPI_UNIT_ASSERT(comm.sync_type() == Communicator::NBX);

    comm.sync_type("sendreceive");
    TIMPI_UNIT_ASSERT(comm.sync_type() == Communicator::SENDRECEIVE);

    comm.sync_type("alltoall");
    TIMPI_UNIT_ASSERT(comm.sync_type() == Communicator::ALLTOALL_COUNTS);
  }

void run_tests()
{
  testPush();
  testPushMove();
  testPull();
  testPushVecVec();
  testPullVecVec();
  testPushMultimap();
  testPushMultimapVecVec();

  testEmptyEntry();

  // Our sync functions need to support sending to ranks that don't
  // exist!  If we're on N processors but working on a mesh
  // partitioned into M parts with M > N, then subpartition p belongs
  // to processor p%N.  Let's make M > N for these tests.
  testPushOversized();
  testPullOversized();
  testPushVecVecOversized();
  testPullVecVecOversized();
  testPushMultimapOversized();
  testPushMultimapVecVecOversized();

  testStringSyncType();
}

int main(int argc, const char * const * argv)
{
  TIMPI::TIMPIInit init(argc, argv);
  TestCommWorld = &init.comm();

  run_tests();

  TestCommWorld->sync_type(Communicator::ALLTOALL_COUNTS);
  run_tests();

  TestCommWorld->sync_type(Communicator::SENDRECEIVE);
  run_tests();

  return 0;
}
