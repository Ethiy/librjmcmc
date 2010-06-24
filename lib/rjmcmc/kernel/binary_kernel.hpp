#ifndef BINARY_KERNEL_HPP_
#define BINARY_KERNEL_HPP_

#include "rjmcmc/random_variant_init.hpp"

namespace rjmcmc {

template<typename Kernel0, typename Kernel1> class binary_kernel
{
  typedef boost::variate_generator<rjmcmc::generator&, boost::uniform_real<> > die_t;
  double m_p, m_q[2];
  Kernel0 m_kernel0;
  Kernel1 m_kernel1;
  mutable unsigned int m_kernel_id;
  mutable die_t m_die;
public:
  enum { size = 2 };
  inline unsigned int kernel_id() const { return m_kernel_id; }
  
  binary_kernel(const Kernel0& k0, const Kernel1& k1, double q0=0.5, double q1=0.5) :
    m_die(random(), boost::uniform_real<>(0,1)), m_kernel0(k0), m_kernel1(k1), m_p(q0+q1)
  {
    double q = q0/m_p;
    m_q[0]=  q;
    m_q[1]=1-q;
  }
  
  inline void probability(double p) { m_p = p; }
  inline double probability() const { return m_p; }
  
  template<typename Configuration, typename Modification>
    double operator()(Configuration& c, Modification& modif) const
  {
    if(m_die()<m_q[0]) {
      m_kernel_id = 0;
      double x = m_q[0]*m_kernel0            (c,modif); // pdf of the sampled modification
      double y = m_q[1]*m_kernel1.reverse_pdf(c,modif); // pdf of the reverse modification
      return y/x;
    } else {
      m_kernel_id = 1;
      double x = m_q[1]*m_kernel1            (c,modif); // pdf of the sampled modification
      double y = m_q[0]*m_kernel0.reverse_pdf(c,modif); // pdf of the reverse modification
      return y/x;
    }
  }
};


class uniform_death_kernel
{
  typedef boost::variate_generator<rjmcmc::generator&, boost::uniform_smallint<> > die_type;
public:
  template<typename Configuration, typename Modification>
    double operator()(Configuration& c, Modification& modif) const
  {
    modif.clear();
    if(c.empty()) return 0.;
    typename Configuration::iterator it = c.begin();
    die_type die(random(), boost::uniform_smallint<>(0,c.size()-1));
    std::advance(it, die());
    modif.insert_death(it);
    return 1./c.size();
  }
  template<typename Configuration, typename Modification>
    double reverse_pdf(const Configuration& c, const Modification& modif) const
  {
    return (modif.birth_size()==1 && modif.death_size()==0)
      ? (1./(c.size()+1))
      : (0.);
  }
};

template<typename Generator>
  class uniform_birth_kernel {
    Generator m_generator;
  public:
    uniform_birth_kernel(const Generator& generator) : 
      m_generator(generator) {}
    
    template<typename Configuration, typename Modification>
      double operator()(Configuration& c, Modification& modif) const
    {
      typedef typename Configuration::value_type T;
      T res;
      random_variant_init(res);
      double p = rjmcmc::apply_visitor(m_generator,res);
      modif.insert_birth(res);
      return p;
    }
    
    template<typename Configuration, typename Modification>
      double reverse_pdf(const Configuration& c, const Modification& modif) const
    {
      return (modif.death_size()==1 && modif.birth_size()==0)
        ? apply_visitor(m_generator.pdf(),c[*modif.death_begin()])
        : (0.);
    }
  };

}; // namespace rjmcmc

#endif // BINARY_KERNEL_HPP_