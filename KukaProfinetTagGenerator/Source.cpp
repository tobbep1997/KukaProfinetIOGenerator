#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <fstream>
#include <ctype.h>

#ifdef _DEBUG
	#define DBG_NEW new (_NORMAL_BLOCK , __FILE__ , __LINE__)
	#define _CRTDBG_MAP_ALLOC
	
	#include <cstdlib>
	#include <crtdbg.h>
#else
	#define DBG_NEW new
#endif // _DEBUG

enum IOType
{
	in,
	out
};

std::string IOType_to_string(IOType type)
{
	switch (type)
	{
	case in:
		return "IN";
		break;
	case out:
		return "OUT";
		break;
	default:
		break;
	}
}

enum SiemensType
{
	Word,
	Int,
	Bool,
	DWord,
	DInt,
	UInt,
	UDInt,
	Byte
};

std::string SiemensType_to_string(SiemensType type)
{
	switch (type)
	{
	case Word:
		return "Word";
		break;
	case Int:
		return "Int";
		break;
	case Bool:
		return "Bool";
		break;
	case DWord:
		return "DWord";
		break;
	case DInt:
		return "DInt";
		break;
	case UInt:
		return "UInt";
		break;
	case UDInt:
		return "UDInt";
		break;
	case Byte:
		return "Byte";
		break;
	default:
		break;
	}
}

enum SignalType
{
	Digital,
	Signed,
	Unsinged
};

std::string SignalType_to_string(SignalType type)
{
	switch (type)
	{
	case Digital:
		return "Digital";
		break;
	case Signed:
		return "Signed";
		break;
	case Unsinged:
		return "Unsinged";
		break;
	default:
		break;
	}
}

struct UserDefinedFieldbusSignal
{
public:
	IOType type;
	SignalType signalType;
	SiemensType siemensType;

	std::string signalName;

	unsigned int address;
	unsigned int bitLenght;

	std::string to_string()
	{
		std::string IOType = IOType_to_string(type);
		std::string SignalType = SignalType_to_string(signalType);
		std::string SiemensType = SiemensType_to_string(siemensType);


		return "IOType : " + IOType + "\nSignalType : " + SignalType + "\nSiemensType : " + SiemensType + "\nSignalName : " + signalName + "\nAddress : " + std::to_string(address) + "\nBitLenght : " + std::to_string(bitLenght);
	}
};

struct IO
{
public:
	IOType type;
	SiemensType siemensType;
	int address;
	std::string name;

	std::string profinetAddress;


	std::string to_string()
	{
		std::string IOType = IOType_to_string(type);
		std::string SiemensType = SiemensType_to_string(siemensType);
	
		return "SignalName : " + name + "\nIOType : " + IOType + "\nSiemensType : " + SiemensType + "\nAddress : " + std::to_string(address) + "\nprofinetAddress : " + profinetAddress;
	}
};



void ReadFile(std::string inFileName, int inStartAddress, int outStartAddress)
{
	std::ifstream inFile(inFileName);

	std::string buffer;

	std::vector<UserDefinedFieldbusSignal*> userDefSignals;
	std::vector<IO*> mappedIO;

	if (inFile.is_open())
	{
		while (std::getline(inFile, buffer))
		{
			if (buffer.find("UserDefinedFieldbusSignal ") != std::string::npos)
			{
				UserDefinedFieldbusSignal* signal = DBG_NEW UserDefinedFieldbusSignal();
				signal->type = buffer.find("Input") != std::string::npos ? IOType::in : IOType::out;
				
				if (buffer.find("Digital") != std::string::npos)
					signal->signalType = SignalType::Digital;
				if (buffer.find("Signed") != std::string::npos)
					signal->signalType = SignalType::Signed;
				if (buffer.find("Unsigned") != std::string::npos)
					signal->signalType = SignalType::Unsinged;

				size_t signalNameStart = buffer.find("NewSignal") + 11;
				size_t signalNameEnd = buffer.find("BitLength") - 2;

				signal->signalName = buffer.substr(signalNameStart, signalNameEnd - signalNameStart);

				size_t addressStart = buffer.find("OriginalSignal") + 22;
				size_t addressEnd = addressStart + 4;

				signal->address = std::atoi(buffer.substr(addressStart, addressEnd).c_str());

				size_t bitLenghtStart = buffer.find("BitLength") + 11;
				size_t bitLenghtEnd = buffer.find("SignalType") - 2;

				signal->bitLenght = std::atoi(buffer.substr(bitLenghtStart, bitLenghtEnd - bitLenghtStart).c_str());

				switch (signal->signalType)
				{
				case SignalType::Digital:
					if (signal->bitLenght == 1)
						signal->siemensType = SiemensType::Bool;
					if (signal->bitLenght == 8)
						signal->siemensType = SiemensType::Byte;
					if (signal->bitLenght == 16)
						signal->siemensType = SiemensType::Word;
					if (signal->bitLenght == 32)
						signal->siemensType = SiemensType::DWord;
					break;
				case SignalType::Signed:
					if (signal->bitLenght == 1)
						signal->siemensType = SiemensType::Bool;
					if (signal->bitLenght == 8)
						signal->siemensType = SiemensType::Byte;
					if (signal->bitLenght == 16)
						signal->siemensType = SiemensType::Int;
					if (signal->bitLenght == 32)
						signal->siemensType = SiemensType::DWord;
					break;
				case SignalType::Unsinged:
					if (signal->bitLenght == 1)
						signal->siemensType = SiemensType::Bool;
					if (signal->bitLenght == 8)
						signal->siemensType = SiemensType::Byte;
					if (signal->bitLenght == 16)
						signal->siemensType = SiemensType::UInt;
					if (signal->bitLenght == 32)
						signal->siemensType = SiemensType::UDInt;
					break;
				default:
					break;
				}

				userDefSignals.push_back(signal);

			
			}

			if (buffer.find("Connection ") != std::string::npos)
			{
				IO* io = DBG_NEW IO();
				size_t start = 0;
				
				bool UserDefSignal = !isdigit(buffer.substr(start = buffer.find("PROFINET/") + 9, start + 1)[0]);
				if (UserDefSignal)
				{
					size_t signalNameStart = start;
					size_t signalNameEnd = buffer.find("ToSignalSystemName") - 2;
					std::string signalName = buffer.substr(signalNameStart, signalNameEnd - signalNameStart);
					
					for (size_t i = 0; i < userDefSignals.size(); i++)
					{
						if (userDefSignals[i]->signalName == signalName)
						{
							io->name = userDefSignals[i]->signalName;
							io->address = userDefSignals[i]->address;
							io->type = userDefSignals[i]->type;
							io->siemensType = userDefSignals[i]->siemensType;							

						}
					}
				}
				else
				{
					size_t addressStart = buffer.find("PROFINET/") + 15;
					size_t addressEnd = buffer.find("ToSignalSystemName") - 2;
					int address = std::atoi(buffer.substr(addressStart, addressEnd).c_str());

					io->type = buffer.find("Input") != std::string::npos ? IOType::in : IOType::out;
					io->name = "Profinet " + IOType_to_string(io->type) + " " + std::to_string(address);
					io->address = address;
					io->siemensType = SiemensType::Bool;
				}

				/*
				B = Byte
				W = Word
				D = DWord
				*/

				
				std::string prefix = io->type != IOType::in ? "%I" : "%Q";
				
				switch (io->siemensType)
				{
				case Word:
					prefix.append("W");
					break;
				case Int:
					prefix.append("W");
					break;
				case DWord:
					prefix.append("D");
					break;
				case DInt:
					prefix.append("D");
					break;
				case UInt:
					prefix.append("W");
					break;
				case UDInt:
					prefix.append("D");
					break;
				case Byte:
					prefix.append("B");
					break;
				default:
					break;
				}
				

				if (io->siemensType == SiemensType::Bool)
					io->profinetAddress = io->type != IOType::in ? "%I" + std::to_string((io->address / 8) + inStartAddress) + "." + std::to_string(io->address % 8) : "%Q" + std::to_string((io->address / 8) + outStartAddress) + "." + std::to_string(io->address % 8);
				else
					io->profinetAddress = io->type != IOType::in ? prefix + std::to_string(io->address + inStartAddress) : prefix + std::to_string(io->address + outStartAddress);
				mappedIO.push_back(io);
			}
		}
	}

	/*
		Tag name;Enter Tag table here		
		Tag;=$B$1;Bool;%Q0.0
	*/

	std::ofstream outFile("output.csv");

	if (outFile.is_open())
	{
		outFile << "Tag name;Default tag table" << std::endl;
	
		for (size_t i = 0; i < mappedIO.size(); i++)
		{
			outFile << mappedIO[i]->name << ";" << "=$B$1;" << SiemensType_to_string(mappedIO[i]->siemensType) << ";" << mappedIO[i]->profinetAddress << std::endl;
		}
	}


	for (size_t i = 0; i < mappedIO.size(); i++)
	{
		std::cout << mappedIO[i]->to_string() << std::endl;
		std::cout << "-----------------------------------------------------------" << std::endl;
		delete mappedIO[i];
	}

	for (size_t i = 0; i < userDefSignals.size(); i++)
	{
		delete userDefSignals[i];
	}
}


int main(int argc, char *argv[])
{
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	std::string inFileName = "";
	
	int outputStartAddress;
	int inputStartAddress;

	if (argc == 4)
	{
		inFileName = argv[1];
		outputStartAddress = atoi(argv[2]);
		inputStartAddress = atoi(argv[3]);
	
		std::cout << "InFileName\t: " + inFileName + "\nOutputAddress\t: " + std::to_string(outputStartAddress) + "\nInputAddress\t: " + std::to_string(inputStartAddress);
	}
	else if (argc == 3)
	{
		inFileName = "IO.xml";
		outputStartAddress = atoi(argv[1]);		
		inputStartAddress = atoi(argv[2]);

		std::cout << "InFileName\t: " + inFileName + "\nOutputAddress\t: " + std::to_string(outputStartAddress) + "\nInputAddress\t: " + std::to_string(inputStartAddress);
	}
	else
	{
		std::cout << "Not enough arguments please input: InFileName, OutputAddress, InputAddress\nOr OutputAddress, InputAddress and the default name will be use (IO.xml)" << std::endl;
	}
	
	std::thread thread(ReadFile, inFileName, inputStartAddress, outputStartAddress);
	thread.join();

	return EXIT_SUCCESS;
}


