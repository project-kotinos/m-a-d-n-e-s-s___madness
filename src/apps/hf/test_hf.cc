#include <iostream>
#include <mra/mra.h>

#include "hartreefock.h"

using std::cout;
using std::endl;

using namespace madness;

const double PI = 3.1415926535897932384;

//*****************************************************************************
double V_func(const Vector<double,3>& r)
{
  double x = r[0];
  double y = r[1];
  double z = r[2];
  return -1.0/(sqrt(x*x + y*y + (z-0.7)*(z-0.7)) + 1e-8) + 
    -1.0/(sqrt(x*x + y*y + (z+0.7)*(z+0.7)) + 1e-8);
}
//*****************************************************************************

//*****************************************************************************
double psi_func(const Vector<double,3>& r)
{
  double x = r[0];
  double y = r[1];
  double z = r[2];
//  return exp(-0.5*(x*x + y*y + (z-0.7)*(z-0.7))) + 
//    exp(-0.5*(x*x + y*y + (z+0.7)*(z+0.7)));
  return exp(-sqrt(x*x + y*y + (z-0.7)*(z-0.7))) + 
    exp(-sqrt(x*x + y*y + (z+0.7)*(z+0.7)));
}
//*****************************************************************************

//*****************************************************************************
template<typename T, int NDIM> class Gaussian :
  public FunctionFunctorInterface<T,NDIM>
{
public:
  typedef Vector<double,NDIM> coordT;
  const coordT center;
  const double exponent;
  const T coefficient;

  Gaussian(const coordT& center, double exponent, T coefficient) :
    center(center), exponent(exponent), coefficient(coefficient)
  {
  }
  
  T operator()(const coordT& x) const
  {
    double sum = 0.0;
    for (int i=0; i<NDIM; i++)
    {
      double xx = center[i]-x[i];
      sum += xx*xx;
    };
    return coefficient*exp(-exponent*sum);
  }
  
};
//*****************************************************************************

//*****************************************************************************
class H2Potential :
  public FunctionFunctorInterface<double,3>
{
public:
  typedef Vector<double,3> coordT;
  
  H2Potential() {}

  double operator()(const coordT& x) const
  {
    double xx = x[0];
    double yy = x[1];
    double zz = x[2];
    return -1.0/(sqrt(xx*xx + yy*yy + (zz-0.7)*(zz-7.0)) + 1e-08) +
      -1.0/(sqrt(xx*xx + yy*yy + (zz+0.7)*(zz+7.0)) + 1e-08);
  }
};
//*****************************************************************************

//*****************************************************************************
template<typename T, int NDIM> class HarmonicOsc3D :
  public FunctionFunctorInterface<T,NDIM>
{
public:
  typedef Vector<double,NDIM> coordT;
  const coordT _center;
  const double _coefficient;
  const double _offset;

  //***************************************************************************
  HarmonicOsc3D(const coordT& center, double coefficient, double offset) :
    _center(center), _coefficient(coefficient), _offset(offset)
  {
  }
  //***************************************************************************

  //***************************************************************************
  T operator()(const coordT& x) const
  {
    double sum = 0.0;
    for (int i=0; i<NDIM; i++)
    {
      double xx = _center[i]-x[i];
      sum += xx*xx;
    };
    return (_coefficient * sum) + _offset;
  }
  //***************************************************************************

};
//*****************************************************************************

//*****************************************************************************
void test_hf_ho(World& world)
{
  cout << "Running test application HartreeFock ..." << endl;
  
  typedef Vector<double,3> coordT;
  typedef SharedPtr< FunctionFunctorInterface<double,3> > functorT;

  // Dimensions of the bounding box
  double bsize = 10.0;
  for (int i=0; i<3; i++)
  {
    FunctionDefaults<3>::cell(i,0) = -bsize;
    FunctionDefaults<3>::cell(i,1) = bsize;
  }
  // Function defaults
  FunctionDefaults<3>::k = 5;
  FunctionDefaults<3>::thresh = 1e-3;
  FunctionDefaults<3>::refine = true;
  FunctionDefaults<3>::initial_level = 2;
  
  // Nuclear potential (harmonic oscillator)
  const coordT origin(0.0);
  const double coeff = 0.5;
  const double offset = -50.0;
  functorT Vnuc_functor(new HarmonicOsc3D<double,3>(origin, coeff, offset));
  Function<double,3> Vnuc = FunctionFactory<double,3>(world).functor(Vnuc_functor);
  
  // Guess for the wavefunction
  functorT wavefunc_functor(new Gaussian<double,3>(origin, -0.5, 100.0));
  Function<double,3> psi = FunctionFactory<double,3>(world).functor(Vnuc_functor);
  psi.scale(1.0/psi.norm2());
  printf("Norm of psi = %.5f\n\n", psi.norm2());
  // Create HartreeFock object
  cout << "Creating HartreeFock object..." << endl;
  HartreeFock hf(world, Vnuc, psi, -42.5, false, false, 1e-5);
  cout << "Running HartreeFock object..." << endl;
  hf.hartree_fock(10);
  printf("Ground state is: %.5f\n", hf.get_eig(0));

}
//*****************************************************************************

//*****************************************************************************
void test_hf_h2(World& world)
{
  cout << "Running test application HartreeFock ..." << endl;
  
  typedef Vector<double,3> coordT;
  typedef SharedPtr< FunctionFunctorInterface<double,3> > functorT;

  // Dimensions of the bounding box
  double bsize = 22.4;
  for (int i=0; i<3; i++)
  {
    FunctionDefaults<3>::cell(i,0) = -bsize;
    FunctionDefaults<3>::cell(i,1) = bsize;
  }
  // Function defaults
  FunctionDefaults<3>::k = 7;
  FunctionDefaults<3>::thresh = 1e-5;
  FunctionDefaults<3>::refine = true;
  FunctionDefaults<3>::initial_level = 2;
  
  // Nuclear potential (harmonic oscillator)
  const coordT origin(0.0);
  cout << "Creating Function object for nuclear potential ..." << endl;
  Function<double,3> Vnuc = FunctionFactory<double,3>(world).f(V_func);
 
  // Guess for the wavefunction
  cout << "Creating wavefunction psi ..." << endl;
  Function<double,3> psi = FunctionFactory<double,3>(world).f(psi_func);
  psi.scale(1.0/psi.norm2());
  printf("Norm of psi = %.5f\n\n", psi.norm2());
  // Create HartreeFock object
  cout << "Creating HartreeFock object..." << endl;
  HartreeFock hf(world, Vnuc, psi, -0.6, true, true, 1e-5);
  cout << "Running HartreeFock object..." << endl;
  hf.hartree_fock(10);
  printf("Ground state is: %.5f\n", hf.get_eig(0));

}
//*****************************************************************************


#define TO_STRING(s) TO_STRING2(s)
#define TO_STRING2(s) #s


//*****************************************************************************
int main(int argc, char** argv)
{
  MPI::Init(argc, argv);
  World world(MPI::COMM_WORLD);
  if (world.rank() == 0)
  {
    print("");
    print("--------------------------------------------");
    print("   MADNESS", MADNESS_VERSION, "multiresolution testsuite");
    print("--------------------------------------------");
    print("");
    print("   number of processors ...", world.size());
    print("    processor frequency ...", cpu_frequency());
    print("            host system ...", TO_STRING(HOST_SYSTEM));
    print("             byte order ...", TO_STRING(MADNESS_BYTE_ORDER));
    print("          configured by ...", MADNESS_CONFIGURATION_USER);
    print("          configured on ...", MADNESS_CONFIGURATION_HOST);
    print("          configured at ...", MADNESS_CONFIGURATION_DATE);
    print("                    CXX ...", MADNESS_CONFIGURATION_CXX);
    print("               CXXFLAGS ...", MADNESS_CONFIGURATION_CXXFLAGS);
#ifdef WORLD_WATCHDOG
    print("               watchdog ...", WATCHDOG_BARK_INTERVAL,
        WATCHDOG_TIMEOUT);
#endif
#ifdef OPTERON_TUNE
    print("             tuning for ...", "opteron");
#elif defined(CORE_DUO_TUNE)
    print("             tuning for ...", "core duo");
#else
    print("             tuning for ...", "core2");
#endif
#ifdef BOUNDS_CHECKING
    print(" tensor bounds checking ...", "enabled");
#endif
#ifdef TENSOR_INSTANCE_COUNT
    print("  tensor instance count ...", "enabled");
#endif
    print(" ");
  }

  try
  {
    printf("WSTHORNTON: Starting up the world ... \n");
    
    startup(world,argc,argv);
    if (world.rank() == 0) print("Initial tensor instance count", BaseTensor::get_instance_count());
    test_hf_h2(world);
  }
  catch (const MPI::Exception& e)
  {
    //        print(e);
    error("caught an MPI exception");
  }
  catch (const madness::MadnessException& e)
  {
    print(e);
    error("caught a MADNESS exception");
  }
  catch (const madness::TensorException& e)
  {
    print(e);
    error("caught a Tensor exception");
  }
  catch (const char* s)
  {
    print(s);
    error("caught a string exception");
  }
  catch (const std::string& s)
  {
    print(s);
    error("caught a string (class) exception");
  }
  catch (const std::exception& e)
  {
    print(e.what());
    error("caught an STL exception");
  }
  catch (...)
  {
    error("caught unhandled exception");
  }

  if (world.rank() == 0)
    print("entering final fence");
  world.gop.fence();
  if (world.rank() == 0)
    print("done with final fence");
  if (world.rank() == 0)
    print("Final tensor instance count", BaseTensor::get_instance_count());
  MPI::Finalize();

  return 0;
}
//*****************************************************************************
