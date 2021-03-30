#include "Engine.h"
#include "Blam/Engine/Objects/Objects.h"

namespace Engine
{
	namespace Objects
	{
		//Grabs object from object table and verifies the type matches
		char* __cdecl try_and_get_data_with_type(datum object_index, int object_type_flags)
		{
			//LOG_TRACE_GAME("call_get_object( object_datum_index: %08X, object_type: %08X )", object_datum_index, object_type);
			typedef char* (__cdecl get_object)(datum object_index, int object_type_flags);
			auto p_get_object = Memory::GetAddress<get_object*>(0x1304E3, 0x11F3A6);
			return p_get_object(object_index, object_type_flags);
		}

		void __cdecl create_new_placement_data(ObjectPlacementData* s_object_placement_data, datum object_definition_index, datum object_owner, int unk)
		{
			//LOG_TRACE_GAME("object_placement_data_new(s_object_placement_data: %08X,",s_object_placement_data);
			//LOG_TRACE_GAME("object_definition_index: %08X, object_owner: %08X, unk: %08X)", object_definition_index, object_owner, unk);

			typedef void(__cdecl object_placement_data_new)(void*, datum, datum, int);
			auto pobject_placement_data_new = Memory::GetAddress<object_placement_data_new*>(0x132163, 0x121033);

			pobject_placement_data_new(s_object_placement_data, object_definition_index, object_owner, unk);
		}

		//Pass new placement data into Create_object_new
		int __cdecl call_object_new(ObjectPlacementData* pObject)
		{
			//LOG_TRACE_GAME("object_new(pObject: %08X)", pObject);

			typedef int(__cdecl object_new)(void*);
			auto p_object_new = Memory::GetAddress<object_new*>(0x136CA7, 0x125B77);

			return p_object_new(pObject);
		}

		//Pass datum from new object into object to sync
		bool __cdecl add_object_to_sync(datum object_index)
		{
			typedef int(__cdecl add_object_to_sync)(datum object_index);
			auto p_add_object_to_sync = Memory::GetAddress<add_object_to_sync*>(0x1B8D14, 0x1B2C44);

			return p_add_object_to_sync(object_index);
		}

		//Fills the creation data for the given object
		int __cdecl set_creation_data(datum object_index, void* creation_data)
		{
			typedef int(__cdecl fill_creation_data_from_object_index)(datum datum, void* creation_data);
			auto p_fill_creation_data_from_object_index = Memory::GetAddress<fill_creation_data_from_object_index*>(0x1F24ED);

			return p_fill_creation_data_from_object_index(object_index, creation_data);
		}


		int get_player_index_from_datum(datum unit_index)
		{
			return (Objects::getObject<BipedObjectDefinition*>(unit_index))->PlayerDatum.ToAbsoluteIndex();
		}
	}
}
