#pragma once
#include "Blam/Cache/DataTypes/DatumIndex.h"
#include "Blam/Engine/Objects/ObjectPlacementData.h"
#include "Blam/Engine/DataArray/DataArray.h"

namespace Engine
{
	game_life_cycle get_game_life_cycle();
	bool __cdecl IsGameMinimized();

	namespace Unit
	{
		int __cdecl remove_equipment(datum unit_index);
		signed int __cdecl inventory_next_weapon(unsigned short unit);
		bool __cdecl assign_equipment_to_unit(datum unit_index, int object_index, short unk);
		int get_actor_datum_from_unit_datum(datum unit_index);
	}

	namespace Objects
	{
		static s_datum_array* getArray()
		{
			//Grabs the pointer from the engine to the s_datum_array of the objects table
			return *Memory::GetAddress<s_datum_array**>(0x4E461C, 0x50C8EC);
		}
		template <typename T = void>
		static T getObject(datum objectDatum)
		{
			if (!getArray()->index_valid(objectDatum))
				return nullptr;
			auto objectPtr = *(char**)(&getArray()->datum[objectDatum.ToAbsoluteIndex() * getArray()->datum_element_size] + 8);
			return reinterpret_cast<T>(objectPtr);
		}

		char* __cdecl try_and_get_data_with_type(datum object_index, int object_type_flags);
		void __cdecl create_new_placement_data(ObjectPlacementData* s_object_placement_data, datum object_definition_index, datum object_owner, int unk);
		int __cdecl call_object_new(ObjectPlacementData* pObject);
		bool __cdecl add_object_to_sync(datum object_index);
		int __cdecl set_creation_data(datum object_index, void* creation_data);
		int get_player_index_from_datum(datum unit_index);
	}
}
