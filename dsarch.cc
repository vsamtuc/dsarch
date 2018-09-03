
#include <iostream>
#include <sstream>
#include <vector>
#include <cassert>

#include <boost/core/demangle.hpp>

#include "dsarch.hh"

namespace dsarch {

using namespace std;

//-------------------
//
//  host
//
//-------------------



std::string named::anon(named const * ptr)
{
        using namespace std;
        ostringstream S;
        S << "<" << boost::core::demangle(typeid(*ptr).name()) << "@"<< ptr << ">";
        return S.str();
}

host::host(network* n, bool _b) 
: _net(n), _addr(unknown_addr), _mcast(_b)
{
	if(!_mcast) {
		_net->_hosts.insert(this);
	}
	else
		_net->_groups.insert(this);
}

host::host(network* n)
: host(n, false)
{ }

host::~host()
{ 
	// nullify incoming channels
	for(auto c: _incoming)
		c->dst = nullptr;

	// remove from network
	if(!_mcast) {
		_net->_hosts.erase(this);
	}
	else
		_net->_groups.erase(this);
}


host_addr host::addr() 
{
	if(_addr == unknown_addr) {
		set_addr();
		if(_addr == unknown_addr)
			this->host::set_addr();
	}
	return _addr;
}


bool host::set_addr(host_addr a)
{
	return _net->assign_address(this, a);
}


void host::set_addr()
{
	if(_addr == unknown_addr) 
		_net->assign_address(this, _addr);
	assert(_addr != unknown_addr);
}


//-------------------
//
//  host group
//
//-------------------


host_group::host_group(network* n) 
: host(n, true)
{ }



//-------------------
//
//  channels
//
//-------------------



channel::channel(host* _src, host* _dst, rpcc_t _rpcc) 
	: src(_src), dst(_dst), rpcc(_rpcc), msgs(0), byts(0) 
{  }

channel::~channel()
{

}

void channel::transmit(size_t msg_size)
{
	msgs++;
	byts += msg_size;
}


multicast_channel::multicast_channel(host *s, host_group* d, rpcc_t rpcc)
	: channel(s,d,rpcc), rxmsgs(0), rxbyts(0)
{
}


size_t multicast_channel::messages_received() const { return rxmsgs; }

size_t multicast_channel::bytes_received() const { return rxbyts; }

void multicast_channel::transmit(size_t msg_size)
{
	channel::transmit(msg_size);
	size_t gsize = static_cast<host_group*>(dst)->receivers(src);
	rxmsgs += gsize;
	rxbyts += gsize*msg_size;
}


string channel::repr() const {
	ostringstream ss;
	ss << "[chan " << src->addr() << "->" << dst->addr() << " traffic:"
		<< msgs << "," << byts << "]";
	ss.flush();
	return ss.str();
}

string multicast_channel::repr() const {
	ostringstream ss;
	ss << "[chan " << src->addr() << "->" << dst->addr() << " traffic:"
		<< msgs << "(" << rxmsgs <<  ")," << byts << "(" << rxbyts << ")]";
	ss.flush();
	return ss.str();
}


//-------------------
//
//  basic network
//
//-------------------


channel* network::create_channel(host* src, host* dst, rpcc_t endp) const
{
	if(dst->is_mcast())
		return new multicast_channel(src, static_cast<host_group*>(dst), endp);
	else
	 	return new channel(src, dst, endp);	
}



channel* network::connect(host* src, host* dst, rpcc_t endp)
{
	// check for existing channel
	for(auto chan : dst->_incoming) {
		assert(chan->dst == dst);
		if(chan->src == src && chan->rpcc==endp)
			return chan;
	}

	if(src->is_mcast())
		throw std::logic_error("A channel source cannot be a host group");
	if(dst->is_mcast() && !rpc().get_method(endp).one_way) {
		throw std::logic_error("A broadcast channel on a non-one_way method cannot be created");
	}

	// create new channel
	channel* chan = create_channel(src, dst, endp);

	// add it to places
	_channels.insert(chan);
	dst->_incoming.insert(chan);

	return chan;
}


void network::disconnect(channel* c)
{
	_channels.erase(c);
	if(c->dst)
		c->dst->_incoming.erase(c);
	delete c;
}

rpcc_t network::decl_interface(const std::type_info& ti)
{
	return decl_interface(type_index(ti));
}

rpcc_t network::decl_interface(const type_index& tix)
{
	return decl_interface(boost::core::demangle(tix.name()));
}

rpcc_t network::decl_interface(const string& name)
{
	return rpctab.declare(name);
}



rpcc_t network::decl_method(rpcc_t ifc, const string& name, bool onew)
{
	return rpctab.declare(ifc, name, onew);
}



network::network()
: all_hosts(this)
{ 
	new_group_addr = -1;
	new_host_addr = 0;
	all_hosts.set_addr(-1);
}


bool network::assign_address(host* h, host_addr a)
{
	if(h->_addr != unknown_addr)
		return h->_addr == a;

	if(a==unknown_addr) {
		// assign a default address
		int step = h->is_mcast() ? -1 : 1;
		host_addr& ap = h->is_mcast() ? new_group_addr : new_host_addr;

		while(h->_addr == unknown_addr) {

			auto it = addr_map.find(ap);
			if(it == addr_map.end()) {
				// found it!
				h->_addr = ap;
				addr_map[ap] = h;
			}

			ap += step;
		}
		return true;
	}


	// check that the address is unassigned
	if(addr_map.find(a)==addr_map.end()) {
		h->_addr = a;
		addr_map[a] = h;
		return true;
	} else {
		return false;
	}
	
}


void network::reserve_addresses(host_addr a)
{
	if(a>=0) {
		if(new_host_addr <= a) new_host_addr = a + 1;
	} else {
		if(new_group_addr>=a) new_group_addr = a - 1;
	}
}

host* network::by_addr(host_addr a) const
{
	auto it = addr_map.find(a);
	return it==addr_map.end() ? nullptr : it->second;
}

network::~network()
{	
}



//-------------------
//
//  RPC interface
//
//-------------------



static inline size_t __method_index(rpcc_t rpcc) {
	if((rpcc & RPCC_METH_MASK)==0)
		throw std::invalid_argument("invalid method code");
	return ((rpcc & RPCC_METH_MASK)>>1)-1;
}

rpc_interface::rpc_interface() 
{}

rpc_interface::rpc_interface(rpcc_t _c, const string& _n) 
: rpc_obj(_c), named(_n) 
{}

rpcc_t rpc_interface::declare(const string& mname, bool onew) 
{
	auto it = name_map.find(mname);
	if(it!=name_map.end()) {
		rpc_method& method = methods[it->second];
		assert(method.name()==mname);
		if(method.one_way != onew)
			throw std::logic_error("method redeclared with different way");
		return method.rpcc;
	}

	if(methods.size()<<1 == RPCC_METH_MASK)
		throw std::length_error("too many methods in interface");

	if(mname.size()==0)
		throw std::invalid_argument("empty method name");

	rpcc_t mrpcc = rpcc | ((methods.size()+1)<<1);
	name_map[mname] = methods.size();
	methods.emplace_back(mrpcc, mname, onew);
	return mrpcc;
}

const rpc_method& rpc_interface::get_method(rpcc_t rpcc) const 
{
	return  methods.at( __method_index(rpcc) );
}

const size_t rpc_interface::num_channels() const
{
	size_t cno = 0;
	for(auto& m : methods)
		cno += m.num_channels();
	return cno;
}


rpcc_t rpc_interface::code(const string& mname) const
{
	auto it = name_map.find(mname);
	if(it != name_map.end()) {
		return methods[it->second].rpcc;
	}
	return 0;
}



//-------------------
//
//  RPC protocol
//
//-------------------

static inline size_t __ifc_index(rpcc_t rpcc) {
	if((rpcc & RPCC_IFC_MASK)==0)
		throw std::invalid_argument("invalid interface code");
	return (rpcc >> RPCC_BITS_PER_IFC)-1;
}

rpcc_t rpc_protocol::declare(const string& name) 
{
	auto it = name_map.find(name);
	if(it != name_map.end()) {
		return ifaces[it->second].rpcc;
	} 
	if(name.size()==0)
		throw std::invalid_argument("empty interface name");
	// create new
	rpcc_t irpcc = (ifaces.size()+1) << RPCC_BITS_PER_IFC;
	name_map[name] = ifaces.size();
	ifaces.emplace_back(irpcc, name);
	return irpcc;
}

rpcc_t rpc_protocol::declare(rpcc_t ifc, const string& mname, bool onew) 
{
	return ifaces[__ifc_index(ifc)].declare(mname, onew);
}

const rpc_interface& rpc_protocol::get_interface(rpcc_t rpcc) const 
{
	return ifaces.at(__ifc_index(rpcc));
}

const rpc_method& rpc_protocol::get_method(rpcc_t rpcc) const 
{
	return ifaces.at(__ifc_index(rpcc)).get_method(rpcc);
}


rpcc_t rpc_protocol::code(const string& name) const
{
	auto it = name_map.find(name);
	if(it != name_map.end()) {
		return ifaces[it->second].rpcc;
	}
	return 0;
}

rpcc_t rpc_protocol::code(const type_info& ti) const
{
	return code(boost::core::demangle(ti.name()));
}

rpcc_t rpc_protocol::code(const string& name, const string& mname) const
{
	auto it = name_map.find(name);
	if(it != name_map.end()) {
		return ifaces[it->second].code(mname);
	}
	return 0;
}

rpcc_t rpc_protocol::code(const type_info& ti, const string& mname) const
{
	return code(boost::core::demangle(ti.name()), mname);
}



const rpc_protocol rpc_protocol::empty;


//-------------------
//
//  RPC proxy
//
//-------------------


rpc_proxy::rpc_proxy(size_t ifc, host* _own)
: _r_ifc(ifc), _r_owner(_own)
{ 
	assert(! _own->is_mcast()); 
}


rpc_proxy::rpc_proxy(const string& name, host* _own) 
: rpc_proxy(_own->net()->decl_interface(name), _own) 
{ }

size_t rpc_proxy::_r_register(rpc_call* call) {
	_r_calls.push_back(call);
	if(_r_calls.size()>=1000)
		throw std::runtime_error("proxy size too large (exceeds 1000 calls)");
	return _r_calls.size();
}

void rpc_proxy::_r_connect(host* dst)
{
	assert(dst != _r_owner);
	_r_proc = dst;
	for(auto call : _r_calls) {
		call->connect(dst);
	}
}


//-------------------
//
//  RPC call
//
//-------------------



rpc_call::rpc_call(rpc_proxy* _prx, bool _oneway, const string& _name)
: _proxy(_prx), 
	_endpoint(_prx->_r_owner->net()->decl_method(_prx->_r_ifc,_name, _oneway)), 
	one_way(_oneway)
{
	_prx->_r_register(this);
}

rpc_call::~rpc_call()
{
	network* nw = _proxy->_r_owner->net();
	nw->disconnect(_req_chan);
	if(! one_way)
		nw->disconnect(_resp_chan);

}

void rpc_call::connect(host* dst)
{
	network* nw = _proxy->_r_owner->net();
	host* owner = _proxy->_r_owner;

	assert(dst->is_mcast() <= one_way); 
	_req_chan = nw->connect(owner, dst, _endpoint);
	if(! one_way)
		_resp_chan = nw->connect(dst, owner, _endpoint | RPCC_RESP_MASK);
}




}

