/* ExclusiveItem.h
Copyright (c) 2022 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <memory>
#include <variant>



// This class provides "data template" classes with abstracted access to an object that
// is either a reference to a shared, "stock" data or is a locally customized instance.
template<class Type>
class ExclusiveItem {
public:
	// Construct an empty ExclusiveItem.
	constexpr ExclusiveItem() noexcept = default;

	// Initialize with a stock item.
	explicit ExclusiveItem(const Type *item) noexcept;
	// Initialize with a locally defined item.
	explicit ExclusiveItem(Type &&item);

	ExclusiveItem(ExclusiveItem &&other) = default;
	ExclusiveItem &operator=(ExclusiveItem &&other) = default;
	ExclusiveItem(const ExclusiveItem &other) = default;
	ExclusiveItem &operator=(const ExclusiveItem &other) = default;

	bool IsStock() const noexcept;

	operator bool() const noexcept;
	// Provides access to the underlying pointer. The caller is responsible
	// for avoiding dereferencing nullptr.
	const Type *Ptr() const noexcept;
	// Provides access to the contained item through a pointer. The caller is responsible
	// for avoiding dereferencing nullptr.
	const Type *operator->() const noexcept;
	// Provides access to the contained item. The caller is responsible
	// for avoiding dereferencing nullptr.
	const Type &operator*() const noexcept;

	bool operator==(const ExclusiveItem &other) const noexcept;
	bool operator!=(const ExclusiveItem &other) const noexcept;


private:
	std::variant<std::shared_ptr<const Type>, const Type *> item;
};



template<class Type>
inline ExclusiveItem<Type>::ExclusiveItem(const Type *item) noexcept
	: item{item}
{
}



template<class Type>
inline ExclusiveItem<Type>::ExclusiveItem(Type &&item)
	: item{std::shared_ptr<const Type>{new Type{item}}}
{
}



template<class Type>
inline bool ExclusiveItem<Type>::IsStock() const noexcept
{
	return item.index();
}



template<class Type>
inline ExclusiveItem<Type>::operator bool() const noexcept
{
	return Ptr();
}



template<class Type>
inline const Type *ExclusiveItem<Type>::Ptr() const noexcept
{
	return item.index() ? std::get<1>(item) : std::get<0>(item).get();
}



template<class Type>
inline const Type *ExclusiveItem<Type>::operator->() const noexcept
{
	return Ptr();
}



template<class Type>
inline const Type &ExclusiveItem<Type>::operator*() const noexcept
{
	return *Ptr();
}



template<class Type>
inline bool ExclusiveItem<Type>::operator==(const ExclusiveItem &other) const noexcept
{
	return *this && other ? **this == *other : static_cast<bool>(*this) == static_cast<bool>(other);
}



template<class Type>
inline bool ExclusiveItem<Type>::operator!=(const ExclusiveItem &other) const noexcept
{
	return *this && other ? **this != *other : static_cast<bool>(*this) != static_cast<bool>(other);
}
