#include <iostream>
#include <cstdio>
#include <ctime>
#include <limits>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/random/random_device.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/exponential_distribution.hpp>
#include "adevs/adevs.h"


typedef boost::random::random_device                 SeedGenerator;
typedef boost::random::mt19937                       RNG;
typedef boost::random::exponential_distribution<>    ExpDist;

static const double infinity = std::numeric_limits<double>::max();  
typedef int IO_type;
typedef adevs::Bag<IO_type>           IOBag;
typedef adevs::Atomic<IO_type>        AtomicModel;
typedef adevs::SimpleDigraph<IO_type> NetworkModel;
typedef adevs::Simulator<IO_type>     Simulator;

class Source : public AtomicModel {
public:
  /// constructor
  Source(RNG & rng, double arrivalRate)
  :
    rng_(rng)
  , expDist_(arrivalRate)
  {}
  
  
  /// Internal transition function.
  void delta_int() final {}
  
  void delta_ext(double, const IOBag&) final {};
  void delta_conf(const IOBag&) final {};
  
  void output_func(IOBag& yb) final {
    yb.insert(1);
  }
  
  double ta() final {
    return this->expDist_(this->rng_);
  }
  
  void gc_output(IOBag&) final {}
  
private:
  RNG &   rng_;
  ExpDist expDist_;
};

class Server : public AtomicModel {
public:
  /// constructor
  Server(RNG & rng, double serviceRate)
  : rng_(rng)
  , expDist_(serviceRate)
  , isBusy_(false)
  , remainingServiceTime_(infinity)
  , queueLength_(0u)
  {
    /// Initially start Job
    ++this->queueLength_;
    this->StartNewJob();
  }
  
private:
  void StartNewJob() {
    --this->queueLength_;
    this->isBusy_ = true;
    this->remainingServiceTime_ = this->expDist_(this->rng_);
  }
  
public:
  /// Internal transition function.
  void delta_int() final {
    /// finish a job
    if(this->queueLength_) {
      /// still jobs in the queue
      /// do next job
      this->StartNewJob();
    } else {
      /// no more jobs in the queue
      this->isBusy_ = false;
      this->remainingServiceTime_ = infinity;
    }
  }
  
  void delta_ext(double e, const IOBag& xb) final {
    /// new job arrives
    /// add to queue
    ++this->queueLength_;
    
    /// if server is busy, advance service time
    if(this->isBusy_) {
      this->remainingServiceTime_ -= e;
    } else {
      /// server was idle, start new job
      this->StartNewJob();
    }
  }
  
  void delta_conf(const IOBag& xb) final {
    /// internal transition first
    this->delta_int();
    this->delta_ext(0.0, xb);
  }
  
  void output_func(IOBag& yb) final {
    /// job finishes
    /// output Queue Length
    yb.insert(this->queueLength_);
  }
  
  double ta() final {
    return this->remainingServiceTime_;
  }
  
  void gc_output(IOBag&) final {}
private:
  RNG &         rng_;
  ExpDist       expDist_;
  double        isBusy_;
  double        remainingServiceTime_;
  unsigned int  queueLength_;
};

class Observer : public AtomicModel {
public:
  /// constructor
  Observer(bool & hasReturnedToZero)
  : hasReturnedToZero_(hasReturnedToZero)
  {}
  
  /// Internal transition function.
  void delta_int() final { }
  
  void delta_ext(double e, const IOBag& xb) final {
    if(not *(xb.begin())) {
      /// queue length has dropped to zero
      this->hasReturnedToZero_ = true;
    }
  }
  
  void delta_conf(const IOBag&) final { }
  
  void output_func(IOBag&) final { }
  
  double ta() final {
    return infinity;
  }
  
  void gc_output(IOBag&) final {}

  
  
private:
  bool & hasReturnedToZero_;
};

class Queue : public NetworkModel {
public:
  /// constructor
  Queue(
    RNG & rng
  , double arrivalRate
  , double serviceRate
  , bool & hasReturnedToZero
  ) :
    source_(new Source(rng, arrivalRate))
  , server_(new Server(rng, serviceRate))
  , observer_(new Observer(hasReturnedToZero))
  {
    this->add(this->source_);
    this->add(this->server_);
    this->add(this->observer_);
    
    this->couple(this->source_, this->server_);
    this->couple(this->server_, this->observer_);
  }
  
private:
  Source*   source_;
  Server*   server_;
  Observer* observer_;
};

namespace po = boost::program_options;
namespace pt = boost::property_tree;

int main(int argc, char **argv) {
  double arrivalRate;
  double serviceRate;
  bool hasReturnedToZero = false;
  double simulationTime;
  double currentTime = 0.0;
  
  /// Declare supported options
    
  /// Options only available as command-line options
  po::options_description generic_options("Options");
  generic_options.add_options()
    ("arrivalrate,a"
    , po::value<double>(&arrivalRate)->required(), "arrival rate")
    ("servicerate,s"
    , po::value<double>(&serviceRate)->required(), "service rate")
    ("simulationtime,T"
    , po::value<double>(&simulationTime)->required()
    , "duration of simulation")
    ("verbose", "more output")
    ("help", "display this help and exit")
    ;
  
  po::variables_map options_varmap;

  po::store(
    po::command_line_parser(argc, argv).
      options(generic_options).
      run(),
      options_varmap
  );

  /// ignore all other options if "--help" was given (or none at all)
  if(options_varmap.count("help") || argc < 2) {
    std::cout << "Usage: " << 
    boost::filesystem::path(argv[0]).filename().string()
    << " [--help] <output file>\n";
    std::cout << generic_options << "\n";
    return 1;
  }

  po::notify(options_varmap);

  SeedGenerator seedGenerator;
  RNG rng(seedGenerator);
  Queue* queue(
    new Queue(rng, arrivalRate, serviceRate, hasReturnedToZero)
  );
  Simulator* sim(new Simulator(queue));
  
  while(not hasReturnedToZero and currentTime < simulationTime) {
    currentTime += sim->nextEventTime();
    sim->execNextEvent();
  }
 
  pt::ptree ptRoot;
  ptRoot.put("ArrivalRate", arrivalRate);
  ptRoot.put("ServiceRate", serviceRate);
  ptRoot.put("SimulationTime", simulationTime);
  ptRoot.put("CurrentTime", currentTime);
  ptRoot.put("HasReturnedToZero", hasReturnedToZero);
  
  pt::json_parser::write_json(std::cout, ptRoot);
  std::cout << std::flush;
  
  delete sim;
  return 0;
}
