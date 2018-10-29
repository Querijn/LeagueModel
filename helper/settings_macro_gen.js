// #define SettingsDef1(SettingsName, Var1, Val1) struct SettingsObjectify(SettingsName) : public Ondergrond::SettingsObject\
// 	{\
// 		SettingsObjectify(SettingsName)(const char* a_Name) :\
// 			Val1(#Val1),\
// 			Ondergrond::SettingsObject(a_Name)\
// 		{\
// 			Ondergrond::SettingsObject::m_CurrentSettingsObject = nullptr;\
// 		}\
// 		\
// 		Ondergrond::SettingsVariable<Var1> Val1;\
// 	};\
// 	extern SettingsObjectify(SettingsName) SettingsName;

const fs = require("fs");

let contents = "";
for (let i = 0; i < 32; i++) 
{
    let args = [];
    for (let j = 1; j <= i + 1; j++) 
        args.push(`Var${j}, Val${j}`);

    contents += `#define SettingsDef${i + 1}(SettingsName, ${args.join(", ")}) struct SettingsObjectify(SettingsName) : public Ondergrond::SettingsObject `;
    contents += `{ `;
    contents += `SettingsObjectify(SettingsName)(const char* a_Name) : `;
    
    for (let j = 1; j <= i + 1; j++) 
        contents += `Val${j}(#Val${j}), `;

    contents += `Ondergrond::SettingsObject(a_Name) `;
    contents += `{ `;
    contents += `Ondergrond::SettingsObject::m_CurrentSettingsObject = nullptr; `; 
    contents += `} `;

    for (let j = 1; j <= i + 1; j++) 
        contents += `Ondergrond::SettingsVariable<Var${j}> Val${j}; `;

    contents += `}; `;
    contents += `extern SettingsObjectify(SettingsName) SettingsName; `;
    contents += '\n';
}

fs.writeFileSync("macro.hpp", contents);