#include "Sol.h"
#include "PhysFSStream.h"

Sol::Sol(const std::string_view fileName)
{
	sf::PhysFSStream file(fileName.data());
	if (file.hasError() == true)
	{
		return;
	}

	auto size = (unsigned)file.getSize();
	data.resize(size);
	file.read(data.data(), size);
}

uint8_t Sol::get(size_t index) const
{
	if (index >= data.size())
	{
		return 0;
	}
	else
	{
		return data[index];
	}
}

bool Sol::passable(size_t index) const
{
	if (index >= data.size())
	{
		return 0;
	}
	else
	{
		return !(data[index] & 0x01);
	}
}
