#pragma once

//TODO: put this somewhere else
struct RESULT
{
	enum class CODE : int
	{
		OK,
		FILE_FAILED_TO_OPEN,
		WORLD_HEADER_MAGIC,
		WORLD_VERSION_INCORRECT,
		WORLD_COMPRESSED,
		WORLD_MESHES_FAILED_TO_LOAD,
		WORLD_EMITTERS_FAILED_TO_LOAD,
		WORLD_SPOT_EFFECTS_FAILED_TO_LOAD,
		WORLD_OBJECTS_FAILED_TO_LOAD,
		WORLD_ACTOR_WAD_FAILED_TO_LOAD,
		WORLD_TEXTURES_ADDRESS_INCORRECT
	} code; //the result code

	RESULT() : code(CODE::OK) { }
	RESULT(CODE code): code(code) { }

	inline bool IsOK() const
	{
		return code == CODE::OK;
	}

	//returns the meaning of the result as a string
	const char *GetMeaning() const
	{
		switch (code)
		{
		case CODE::OK:
			return "No error occured.";
		case CODE::FILE_FAILED_TO_OPEN:
			return "Failed to open the file.";
		case CODE::WORLD_HEADER_MAGIC:
			return "World header magic is incorrect.";
		case CODE::WORLD_VERSION_INCORRECT:
			return "This world file version is not supported.";
		case CODE::WORLD_COMPRESSED:
			return "This level is compressed. Please decompress it first using the EDNDEC utility.";
		case CODE::WORLD_MESHES_FAILED_TO_LOAD:
			return "Unsupported mesh data. Bailing out.";
		case CODE::WORLD_EMITTERS_FAILED_TO_LOAD:
			return "Unsupported emitter data. Bailing out.";
		case CODE::WORLD_SPOT_EFFECTS_FAILED_TO_LOAD:
			return "Unsupported spot effect data. Bailing out.";
		case CODE::WORLD_OBJECTS_FAILED_TO_LOAD:
			return "Unsupported object data. Bailing out.";
		case CODE::WORLD_ACTOR_WAD_FAILED_TO_LOAD:
			return "Failed to load the Actor WAD.";
		case CODE::WORLD_TEXTURES_ADDRESS_INCORRECT:
			return "Invalid textures start address.";
		default:
			return "Unknown error occured.";
		}
	}
};
