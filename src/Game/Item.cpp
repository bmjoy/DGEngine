#include "Item.h"
#include "Game.h"
#include "GameHashes.h"
#include "GameUtils.h"
#include "Game/Level.h"
#include "Player.h"
#include "Utils/Utils.h"

Item::Item(const ItemClass* class__) : class_(class__)
{
	base.texturePack = class__->getDropTexturePack();
	base.animation.textureIndexRange = class__->getDropTextureIndexRange();
	base.animation.textureIndexRange.second--;
	base.animation.currentTextureIdx = base.animation.textureIndexRange.second;
	base.animation.frameTime = class__->AnimationSpeed();
	base.animation.animType = AnimationType::PlayOnce;
	base.hoverCellSize = 1;
	base.sprite.setOutline(class__->Outline(), class__->OutlineIgnore());
	base.updateTexture();
	applyDefaults();
}

void Item::resetDropAnimation(Level& level) noexcept
{
	base.animation.currentTextureIdx = base.animation.textureIndexRange.first;
	if (base.enableHover == true)
	{
		wasHoverEnabledOnItemDrop = true;
		base.enableHover = false;
	}
	if (base.updateTexture() == true)
	{
		base.updateDrawPosition(level);
	}
}

bool Item::getTexture(size_t textureNumber, TextureInfo& ti) const
{
	switch (textureNumber)
	{
	case 0:
		return base.getTexture(ti);
	case 1:
		return class_->getInventoryTexture(*this, ti);
	default:
		return false;
	}
}

void Item::executeAction(Game& game) const
{
	class_->executeAction(game, str2int16("action"));
}

void Item::update(Game& game, Level& level)
{
	base.processQueuedActions(game);
	base.updateHover(game, level, this);

	if (base.hasValidState() == true &&
		base.animation.update(game.getElapsedTime()) == true)
	{
		if (wasHoverEnabledOnItemDrop == true)
		{
			base.enableHover = true;
			wasHoverEnabledOnItemDrop = false;
		}
		if (base.updateTexture() == true)
		{
			base.updateDrawPosition(level);
		}
	}
}

bool Item::getProperty(const std::string_view prop, Variable& var) const
{
	if (prop.empty() == true)
	{
		return false;
	}
	auto props = Utils::splitStringIn2(prop, '.');
	auto propHash = str2int16(props.first);
	switch (propHash)
	{
	case str2int16("class"):
	case str2int16("classId"):
		var = Variable(class_->Id());
		break;
	case str2int16("shortName"):
		var = Variable(ShortName());
		break;
	case str2int16("name"):
	{
		updateNameAndDescriptions();
		var = Variable(Name());
		break;
	}
	case str2int16("simpleName"):
		var = Variable(SimpleName());
		break;
	case str2int16("d"):
	case str2int16("description"):
	{
		updateNameAndDescriptions();
		size_t idx = Utils::strtou(props.second);
		if (idx >= descriptions.size())
		{
			idx = 0;
		}
		var = Variable(descriptions[idx]);
		break;
	}
	case str2int16("hasDescription"):
	{
		updateNameAndDescriptions();
		bool hasDescr = false;
		size_t idx = Utils::strtou(props.second);
		if (idx < descriptions.size())
		{
			hasDescr = descriptions[idx].empty() == false;
		}
		var = Variable(hasDescr);
		break;
	}
	case str2int16("prices"):
	{
		LevelObjValue value;
		if (class_->evalFormula(str2int16(props.second), *this, value) == false)
		{
			if (getIntByHash(propHash, value) == false)
			{
				return false;
			}
		}
		var = Variable((int64_t)value);
		break;
	}
	case str2int16("identified"):
		var = Variable(identified);
		break;
	case str2int16("type"):
		var = Variable(std::string("item"));
		break;
	case str2int16("itemType"):
		var = Variable(ItemType());
		break;
	case str2int16("itemSubType"):
		var = Variable(ItemSubType());
		break;
	case str2int16("isUsable"):
		var = Variable(isUsable());
		break;
	case str2int16("needsRecharge"):
		var = Variable(needsRecharge());
		break;
	case str2int16("needsRepair"):
		var = Variable(needsRepair());
		break;
	case str2int16("propertyCount"):
		var = Variable((int64_t)properties.size());
		break;
	case str2int16("hasProperty"):
		var = Variable(hasInt(props.second));
		break;
	default:
	{
		LevelObjValue value;
		if (getIntByHash(propHash, value) == true)
		{
			var = Variable((int64_t)value);
			break;
		}
		else
		{
			return false;
		}
	}
	}
	return true;
}

void Item::setProperty(const std::string_view prop, const Variable& val)
{
	if (prop.empty() == true)
	{
		return;
	}
	LevelObjValue val2 = 0;
	if (std::holds_alternative<bool>(val) == true)
	{
		val2 = std::get<bool>(val) ? 1 : 0;
	}
	else if (std::holds_alternative<int64_t>(val) == true)
	{
		val2 = (LevelObjValue)std::get<int64_t>(val);
	}
	setIntByHash(str2int16(prop), val2);
}

bool Item::hasIntByHash(uint16_t propHash) const noexcept
{
	return properties.hasValue(propHash);
}

bool Item::hasInt(const std::string_view prop) const noexcept
{
	return hasIntByHash(str2int16(prop));
}

LevelObjValue Item::getIntByHash(uint16_t propHash) const
{
	LevelObjValue value = 0;
	getIntByHash(propHash, value);
	return value;
}

LevelObjValue Item::getInt(const std::string_view prop) const
{
	LevelObjValue value = 0;
	getInt(prop, value);
	return value;
}

bool Item::getIntByHash(uint16_t propHash, LevelObjValue& value) const
{
	switch (propHash)
	{
	case ItemProp::Identified:
		value = (LevelObjValue)identified;
		break;
	case str2int16("indestructible"):
		value = (LevelObjValue)(getIntByHash(ItemProp::DurabilityMax)
			== std::numeric_limits<LevelObjValue>::max());
		break;
	case str2int16("unlimitedCharges"):
		value = (LevelObjValue)(getIntByHash(ItemProp::ChargesMax)
			== std::numeric_limits<LevelObjValue>::max());
		break;
	default:
		return properties.getValue(propHash, value);
	}
	return true;
}

bool Item::getInt(const std::string_view prop, LevelObjValue& value) const
{
	return getIntByHash(str2int16(prop), value);
}

void Item::setIntByHash(uint16_t propHash, LevelObjValue value)
{
	switch (propHash)
	{
	case ItemProp::Identified:
		identified = value != 0;
		break;
	default:
	{
		if (properties.setValue(propHash, value) == false)
		{
			return;
		}
	}
	}
	updateNameAndDescr = true;
}

void Item::setInt(const std::string_view prop, LevelObjValue value)
{
	setIntByHash(str2int16(prop), value);
}

void Item::updateNameAndDescriptions() const
{
	if (updateNameAndDescr == true)
	{
		updateNameAndDescr = false;
		if (identified == false)
		{
			name = SimpleName();
		}
		else
		{
			if (class_->getFullName(*this, name) == false)
			{
				name = SimpleName();
			}
		}
		for (size_t i = 0; i < descriptions.size(); i++)
		{
			class_->getDescription(i, *this, descriptions[i]);
		}
	}
}

void Item::applyDefaults()
{
	for (const auto& prop : class_->Defaults())
	{
		setIntByHash(prop.first, prop.second);
	}
}

bool Item::needsRecharge() const
{
	LevelObjValue charges = 0;
	if (getIntByHash(ItemProp::ChargesMax, charges) == true)
	{
		if (charges < std::numeric_limits<LevelObjValue>::max())
		{
			return getIntByHash(ItemProp::Charges) < charges;
		}
	}
	return false;
}

bool Item::needsRepair() const
{
	LevelObjValue durability = 0;
	if (getIntByHash(ItemProp::DurabilityMax, durability) == true)
	{
		if (durability < std::numeric_limits<LevelObjValue>::max())
		{
			return getIntByHash(ItemProp::Durability) < durability;
		}
	}
	return false;
}

bool Item::isUsable() const noexcept
{
	return hasIntByHash(ItemProp::UseOn);
}

bool Item::useHelper(uint16_t propHash, uint16_t useOpHash,
	LevelObjValue value, Player& player, const Level* level) const noexcept
{
	Number32 origValue2;
	if (player.getNumberByHash(propHash, origValue2) == false)
	{
		return false;
	}
	auto origValue = origValue2.getInt64();

	switch (useOpHash)
	{
	default:
	case str2int16("+"):
		origValue += value;
		break;
	case str2int16("-"):
		origValue -= value;
		break;
	case str2int16("="):
		origValue = value;
		break;
	}
	return player.setNumberByHash(propHash, (LevelObjValue)origValue, level);
}

bool Item::useHelper(uint16_t propHash, uint16_t useOpHash, uint16_t valueHash,
	uint16_t valueMaxHash, Player& player, const Level* level) const
{
	LevelObjValue value;
	if (class_->evalFormula(valueHash, *this, player, value) == false)
	{
		return false;
	}
	LevelObjValue valueMax;
	if (class_->evalFormula(valueMaxHash, *this, player, valueMax) == true)
	{
		value = Utils::Random::get<LevelObjValue>(value, valueMax);
	}
	return useHelper(propHash, useOpHash, value, player, level);
}

bool Item::use(Player& player, const Level* level, uint32_t& quantityLeft)
{
	quantityLeft = 0;
	LevelObjValue useOn;
	if (getIntByHash(ItemProp::UseOn, useOn) == false ||
		useOn == str2int16("") ||
		player.canUseItem(*this) == false)
	{
		return false;
	}

	LevelObjValue itemQuantity;
	bool hasQuantity = getIntByHash(ItemProp::Quantity, itemQuantity);
	if (hasQuantity == true &&
		itemQuantity <= 0)
	{
		return true;
	}

	auto propHash = (uint16_t)useOn;
	auto useOp = getIntByHash(ItemProp::UseOp);
	bool ret = false;

	switch (propHash)
	{
	case ItemProp::LifeAndManaDamage:
	{
		ret |= useHelper(ItemProp::LifeDamage, useOp,
			ItemProp::Value, ItemProp::ValueMax, player, level);
		ret |= useHelper(ItemProp::ManaDamage, useOp,
			ItemProp::Value2, ItemProp::Value2Max, player, level);
		break;
	}
	case ItemProp::AllAttributes:
	{
		LevelObjValue value;
		if (class_->evalFormula(ItemProp::Value, *this, player, value) == false)
		{
			break;
		}
		LevelObjValue valueMax;
		if (class_->evalFormula(ItemProp::ValueMax, *this, player, valueMax) == true)
		{
			value = Utils::Random::get<LevelObjValue>(value, valueMax);
		}
		ret |= useHelper(ItemProp::Strength, useOp, value, player, level);
		ret |= useHelper(ItemProp::Magic, useOp, value, player, level);
		ret |= useHelper(ItemProp::Dexterity, useOp, value, player, level);
		ret |= useHelper(ItemProp::Vitality, useOp, value, player, level);
		break;
	}
	default:
	{
		ret = useHelper(propHash, useOp, ItemProp::Value, ItemProp::ValueMax, player, level);
		break;
	}
	}
	if (ret == true)
	{
		if (hasQuantity == true)
		{
			itemQuantity--;
			setIntByHash(ItemProp::Quantity, itemQuantity);
			quantityLeft = (uint32_t)itemQuantity;
			player.updateItemQuantityCache(ItemProp::Quantity);
		}
		player.updateProperties();
	}
	return ret;
}

LevelObjValue Item::addQuantity(LevelObjValue& amount)
{
	LevelObjValue capacity;
	if (amount == 0 ||
		getIntByHash(ItemProp::Capacity, capacity) == false)
	{
		return 0;
	}
	auto quantity = getIntByHash(ItemProp::Quantity);
	if (amount < 0)
	{
		auto absAmount = std::abs(amount);
		if (absAmount <= quantity)
		{
			auto newQuant = quantity - absAmount;
			setIntByHash(ItemProp::Quantity, newQuant);
			amount = 0;
			return newQuant;
		}
		else
		{
			setIntByHash(ItemProp::Quantity, 0);
			amount += quantity;
			return 0;
		}
	}
	else
	{
		if (amount + quantity <= capacity)
		{
			auto newQuant = amount + quantity;
			setIntByHash(ItemProp::Quantity, newQuant);
			amount = 0;
			return newQuant;
		}
		else
		{
			setIntByHash(ItemProp::Quantity, capacity);
			amount -= (capacity - quantity);
			return capacity;
		}
	}
}
