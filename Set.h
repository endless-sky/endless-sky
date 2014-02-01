/* Set.h
Michael Zahniser, 28 Oct 2013

Template representign a set of named objects of a given type, where you can
query it for a pointer to any object and it will return one, whether or not that
object has been loaded yet. (This allows cyclic pointers.)
*/

#ifndef SET_H_INCLUDED
#define SET_H_INCLUDED

#include <map>
#include <string>



template<class Type>
class Set {
public:
	// Allow non-const access to the owner of this set; it can hand off only
	// const references to avoid anyone else modifying the objects.
	Type *Get(const std::string &name) { return &data[name]; }
	const Type *Get(const std::string &name) const { return &data[name]; }
	
	bool Has(const std::string &name) const { return (data.find(name) != data.end()); }
	
	typename std::map<std::string, Type>::iterator begin() { return data.begin(); }
	typename std::map<std::string, Type>::const_iterator begin() const { return data.begin(); }
	typename std::map<std::string, Type>::iterator end() { return data.end(); }
	typename std::map<std::string, Type>::const_iterator end() const { return data.end(); }
	
	
private:
	mutable std::map<std::string, Type> data;
};



#endif
