#include <iostream>
#include "src/exceptions/error.hpp"

int main(){
	try {
		throw zenith::Error({1,5,0,0},"Test Error");
	} catch(const zenith::Error& e){
		std::cout << "Caught: " << e.what() << std::endl;
	}
}