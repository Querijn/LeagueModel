#pragma once

#include <file_system.hpp>

#include <glm/glm.hpp>

#include <vector>
#include <map>
#include <string>

namespace League
{
	class Bin;

	class BaseValueStorage
	{
	public:
		using FindConditionFunction = bool(*)(const BaseValueStorage& a_Value, void* a_UserData);
		
		enum Type : uint8_t
		{
			U16Vec3 = 0,
			Bool = 1,
			S8 = 2,
			U8 = 3,
			S16 = 4,
			U16 = 5,
			S32 = 6,
			U32 = 7,
			S64 = 8,
			U64 = 9,
			Float = 10,
			FVec2 = 11,
			FVec3 = 12,
			FVec4 = 13,
			Mat4 = 14,
			RGBA = 15,
			String = 16,
			Hash = 17,
			Container = 18,
			Struct = 19,
			Embedded = 20,
			Link = 21,
			Array = 22,
			Map = 23,
			Padding = 24,
		};

		BaseValueStorage(Type a_Type, uint32_t a_Hash) : m_Hash(a_Hash), m_Type(a_Type) { }

		virtual std::string DebugPrint() const = 0;
		virtual std::string GetAsJSON(bool a_ExposeHash) const = 0;
		bool Is(const std::string& a_Name) const;

		virtual std::vector<const BaseValueStorage*> Find(FindConditionFunction a_Function, void * a_UserData) const = 0;

		uint32_t GetHash() const { return m_Hash; }
		Type GetType() const { return m_Type; }

		virtual BaseValueStorage* GetChild(const std::string& a_Child) const = 0;
		virtual BaseValueStorage* GetChild(size_t a_Index) const = 0;

		virtual void FetchDataFromFile(File* a_File, size_t& a_Offset) = 0;

		friend class League::Bin;
	protected:
		static BaseValueStorage* Create(Type a_Type, uint32_t a_Hash = 0);
		static BaseValueStorage* Create(File* a_File, size_t& a_Offset);

		std::string GetHashJSONPrefix() const; 

	private:
		uint32_t m_Hash;
		Type m_Type;
	};

	template<typename T>
	class NumberValueStorage : public BaseValueStorage
	{
	public:
		NumberValueStorage(Type a_Type, uint32_t a_Hash) : BaseValueStorage(a_Type, a_Hash) { }

		std::vector<const BaseValueStorage*> Find(FindConditionFunction a_Function, void * a_UserData) const override { return std::vector<const BaseValueStorage*>(); }

		std::string DebugPrint() const override { return std::to_string(m_Data); }
		std::string GetAsJSON(bool a_ExposeHash) const override
		{
			if (a_ExposeHash)
				return GetHashJSONPrefix() + std::to_string(m_Data);
			else return std::to_string(m_Data);
		}

		BaseValueStorage* GetChild(const std::string& a_Child) const override { return nullptr; }
		BaseValueStorage* GetChild(size_t a_Index) const override { return nullptr; }

		T Get() const { return m_Data; }

		void FetchDataFromFile(File* a_File, size_t& a_Offset) override { a_File->Get(m_Data, a_Offset); }

	private:
		T m_Data;
	};

	template<typename T, typename StorageType, typename FileType, int ElementCount>
	class NumberVectorValueStorage : public BaseValueStorage
	{
	public:
		NumberVectorValueStorage(Type a_Type, uint32_t a_Hash) : BaseValueStorage(a_Type, a_Hash) { }

		std::vector<const BaseValueStorage*> Find(FindConditionFunction a_Function, void * a_UserData) const override { return std::vector<const BaseValueStorage*>(); }

		std::string DebugPrint() const override
		{
			std::string t_Result = "{ ";

			bool t_First = true;
			for (int i = 0; i < ElementCount; i++)
			{
				if (!t_First) t_Result += ", ";
				t_Result += std::to_string(m_Data[i]);
				t_First = false;
			}

			return t_Result + " }";
		}

		std::string GetAsJSON(bool a_ExposeHash) const override
		{
			std::string t_Result = "[ ";

			bool t_First = true;
			for (int i = 0; i < ElementCount; i++)
			{
				if (!t_First) t_Result += ", ";
				t_Result += std::to_string(m_Data[i]);
				t_First = false;
			}

			t_Result += " ]";

			if (a_ExposeHash)
				return GetHashJSONPrefix() + t_Result;
			else return t_Result;
		}

		BaseValueStorage* GetChild(const std::string& a_Child) const override { return nullptr; }
		BaseValueStorage* GetChild(size_t a_Index) const override { return nullptr; }

		T Get() const { return m_Data; }

		void FetchDataFromFile(File* a_File, size_t& a_Offset) override
		{
			for (int i = 0; i < ElementCount; i++)
			{
				FileType t_Element;
				a_File->Get(t_Element, a_Offset);
				m_Data[i] = (StorageType)t_Element;
			}
		}

	private:
		T m_Data;
	};

	class StringValueStorage : public BaseValueStorage
	{
	public:
		StringValueStorage(Type a_Type, uint32_t a_Hash) : BaseValueStorage(a_Type, a_Hash) { }

		std::vector<const BaseValueStorage*> Find(FindConditionFunction a_Function, void * a_UserData) const override { return std::vector<const BaseValueStorage*>(); }

		std::string DebugPrint() const override;
		std::string GetAsJSON(bool a_ExposeHash) const override;

		BaseValueStorage* GetChild(const std::string& a_Child) const override { return nullptr; }
		BaseValueStorage* GetChild(size_t a_Index) const override { return nullptr; }

		std::string Get() const { return m_Data; }

		void FetchDataFromFile(File* a_File, size_t& a_Offset) override;

	private:
		std::string m_Data;
	};

	class MatrixValueStorage : public BaseValueStorage
	{
	public:
		MatrixValueStorage(Type a_Type, uint32_t a_Hash) : BaseValueStorage(a_Type, a_Hash) { }

		std::vector<const BaseValueStorage*> Find(FindConditionFunction a_Function, void * a_UserData) const override { return std::vector<const BaseValueStorage*>(); }

		std::string DebugPrint() const override;
		std::string GetAsJSON(bool a_ExposeHash) const override;

		BaseValueStorage* GetChild(const std::string& a_Child) const override { return nullptr; }
		BaseValueStorage* GetChild(size_t a_Index) const override { return nullptr; }

		glm::mat4 Get() const { return m_Data; }

		void FetchDataFromFile(File* a_File, size_t& a_Offset) override;

	private:
		glm::mat4 m_Data;
	};

	class ArrayValueStorage : public BaseValueStorage
	{
	public:
		ArrayValueStorage(Type a_Type, uint32_t a_Hash) : BaseValueStorage(a_Type, a_Hash) { }

		std::vector<const BaseValueStorage*> Find(FindConditionFunction a_Function, void * a_UserData) const override;

		std::string DebugPrint() const override;
		std::string GetAsJSON(bool a_ExposeHash) const override;

		virtual BaseValueStorage* GetChild(const std::string& a_Child) const override { return nullptr; }
		BaseValueStorage* GetChild(size_t a_Index) const override;

		const std::vector<BaseValueStorage*>& Get() const { return m_Data; }

		virtual void FetchDataFromFile(File* a_File, size_t& a_Offset) override;

	protected:
		std::vector<BaseValueStorage*> m_Data;
	};

	class StructValueStorage : public ArrayValueStorage
	{
	public:
		StructValueStorage(Type a_Type, uint32_t a_Hash) : ArrayValueStorage(a_Type, a_Hash) { }

		BaseValueStorage* GetChild(const std::string& a_Child) const override;

		void FetchDataFromFile(File* a_File, size_t& a_Offset) override;
	};

	class ContainerValueStorage : public ArrayValueStorage
	{
	public:
		ContainerValueStorage(Type a_Type, uint32_t a_Hash) : ArrayValueStorage(a_Type, a_Hash) { }

		BaseValueStorage* GetChild(const std::string& a_Child) const override;

		void FetchDataFromFile(File* a_File, size_t& a_Offset) override;
	};

	class MapValueStorage : public BaseValueStorage
	{
	public:
		MapValueStorage(Type a_Type, uint32_t a_Hash) : BaseValueStorage(a_Type, a_Hash) { }

		std::vector<const BaseValueStorage*> Find(FindConditionFunction a_Function, void * a_UserData) const override;

		std::string DebugPrint() const override;
		std::string GetAsJSON(bool a_ExposeHash) const override;

		BaseValueStorage* GetChild(const std::string& a_Child) const override;
		BaseValueStorage* GetChild(size_t a_Index) const override { return nullptr; }

		const std::map<BaseValueStorage*, BaseValueStorage*>& Get() const { return m_Data; }

		void FetchDataFromFile(File* a_File, size_t& a_Offset) override;

	private:
		std::map<BaseValueStorage*, BaseValueStorage*> m_Data;
	};
}