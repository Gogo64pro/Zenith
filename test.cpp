#include "src/core/indirect_polymorphic.hpp"
#include <iostream>

struct D0 {
	virtual void foo() { std::puts("D0::foo"); }
	virtual void bar() = 0;
	virtual ~D0() { std::puts("D0::~D0"); }
	virtual void someFuncMember(std_P3019_modified::polymorphic<D0> name){std::puts("D0::someFuncMember");}
};

struct D1 : D0 {
	void foo() override { std::puts("D1::foo"); }
	// void bar() override {}
	~D1() override { std::puts("D1::~D1"); }
};

struct DImpl : D1 {
	void foo() override { std::puts("DImpl::foo"); }
	void bar() override {}
	~DImpl() override { std::puts("DImpl::~DImpl"); }
	void someFuncMember(std_P3019_modified::polymorphic<D0> name) override {std::puts("DImpl::someFuncMember");}
};
struct DImpl2 : D0{
	void foo() override { std::puts("DImpl2::foo"); }
	void bar() override {}
	void someFuncMember(std_P3019_modified::polymorphic<D0> name) override {std::puts("DImpl2::someFuncMember");}
	~DImpl2() override { std::puts("DImpl2::~DImpl2");};
};
std_P3019_modified::polymorphic<DImpl> someFunc() {
	return std::move(std_P3019_modified::make_polymorphic<DImpl>());
}

int main() {
	using namespace std_P3019_modified;

	std::puts("Creating a");
	polymorphic<DImpl> a = make_polymorphic<DImpl>();
	std::puts("Creating b from a");
	polymorphic<D1> b = a;  // derived-to-base conversion
	std::puts("Creating c from a");
	polymorphic<D0> c = a;  // derived-to-base conversion
	std::puts("Creating d from b");
	polymorphic<D0> d = b;  // derived-to-base conversion

	polymorphic<DImpl2> e = make_polymorphic<DImpl2>();


	a->foo();  // Should print "DImpl::foo"
	b->foo();  // Should print "DImpl::foo"
	c->foo();  // Should print "DImpl::foo"
	d->foo();  // Should print "DImpl::foo"

}
