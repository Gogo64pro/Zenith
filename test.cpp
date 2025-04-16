#include <iostream>
#include "src/exceptions/ParseError.hpp"

int main(){
	try {
		throw zenith::ParseError({1,5,0,0},"Test Error");
	} catch(const zenith::ParseError& e){
		std::cout << "Caught: " << e.what() << std::endl;
	}
}