#include <fstream>
#include "lexer.h"
#include "logger.h"
#include "source_code.h"
#include "parser.h"

int main()
{
	std::ifstream         f(R"(D:\Projects\protolang\test\code.ptl)");
	protolang::SourceCode src(f);
	protolang::Logger     logger(src, std::cout);
	protolang::Lexer      lexer(src, logger);

	while (lexer.lex() == 0)
	{
		 logger.print(lexer.token);
	}

	protolang::Parser parser;


	//logger.print_code_range({0,1}, {1,1});

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
