#include <fstream>
#include "lexer.h"
#include "logger.h"
#include "parser.h"
#include "source_code.h"
#include "token.h"
int main()
{
	try
	{
		std::string file_name        = __FILE__ R"(\..\..\test\test2.ptl)";
		std::string output_file_name = __FILE__ R"(\..\..\test\.dump.json)";

		std::ifstream         f(file_name);
		bool                  file_good = f.good();
		protolang::SourceCode src(f);
		protolang::Logger     logger(src, std::cout);
		protolang::Lexer      lexer(src, logger);

		if (!file_good)
			logger.log(protolang::FatalFileError(file_name, 'r'));

		std::vector<protolang::Token> tokens = lexer.scan();
		if (tokens.empty())
			return 1;

		protolang::Parser parser(tokens, logger);
		auto              prog = parser.parse();
		if (!prog)
			return 1;

		std::ofstream(output_file_name) << prog->dump_json() << "\n";
	}
	catch (protolang::ExceptionFatalError error)
	{
		return 1;
	}
	// logger.print_code_range({0,1}, {1,1});

	//	protolang::Lexer lexer("01");
	//	int              code = lexer.lex();
	//	while (code == 0)
	//	{
	//		std::cout << lexer.token << std::endl;
	//		code = lexer.lex();
	//	}

	//	protolang::Lexer lexer{"12 3 你好 5 6 7 88"};
	//	int result = -1;
	//	while(result != 0)
	//	{
	//		result = lexer.lex();
	//		std::cout << result << std::endl;
	//	}
}

//	KLogEntry e1;
// e1.m_changes = 100;
// e1.m_time = QDateTime(QDate(2011, 1, 1));
//
// KLogEntry e2;
// e2.m_changes = 200;
// e2.m_time = QDateTime(QDate(2022, 2, 2));
//
// KLogEntry e3;
// e3.m_changes = 100;
// e3.m_time = QDateTime(QDate(2033, 3, 3));
//
// KLogEntry e4;
// e4.m_changes = 200;
// e4.m_time = QDateTime(QDate(2044, 4, 4));
//
// qDebug() << e1.m_time << e2.m_time << e3.m_time << e4.m_time;
//
// KList<KLogEntry> l1 = {e1, e2};
// KList<KLogEntry> l2 = {e3, e4};
// KList<KLogEntry> l3 = {e1, e2, e3};
//
// KList<KList<KLogEntry>> listOflist{l1, l2, l3};
//
// QJsonValue val;
// KJson::toJson(&listOflist, val);
//
// QJsonObject root;
// root.insert("root", val);
//
// QJsonDocument doc;
// doc.setObject(root);
// std::string out = doc.toJson().toStdString();
// std::cout << out;
//
// KList<KList<KLogEntry>> listOut;
// KJson::fromJson(val, &listOut);
//
// for (int i = 0; i < listOut.size(); i++)
//{
//	qDebug() << "lst" << i;
//	for (int j = 0; j < listOut[i].size(); j++)
//	{
//		qDebug() << "	" << listOut[i][j].m_changes << listOut[i][j].m_time;
//	}
//}
