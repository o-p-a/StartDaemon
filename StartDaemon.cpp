/*
	Start Daemon
	(separate stdin,out,err and invoke CreateProcess)

	by opa
*/

#include <string>
#include <algorithm>
#include <cstdio>

#include <windows.h>

#define PGM					"StartDaemon"
#define PGM_DEBUG			PGM ": "
#define PGM_INFO			PGM ": "
#define PGM_WARN			PGM " warning: "
#define PGM_ERR				PGM " error: "
#define VERSTR				"1.00"

#define CREDIT2009			"Copyright (c) 2009 by opa"

typedef signed char schar;
typedef unsigned char uchar;
typedef signed int sint;
typedef unsigned int uint;
typedef signed long slong;
typedef unsigned long ulong;

using namespace std;

char
	credit[] = PGM " version " VERSTR " " CREDIT2009;
bool
	error = false;
sint
	rcode = 0;

////////////////////////////////////////////////////////////////////////

template <class BidirectionalIterator, class T>
BidirectionalIterator rfind(BidirectionalIterator first, BidirectionalIterator last, const T &value)
{
	while(first != last){
		--last;
		if(*last == value)
			break;
	}

	return last;
}

template <class BidirectionalIterator, class Predicate>
BidirectionalIterator rfind_if(BidirectionalIterator first, BidirectionalIterator last, Predicate pred)
{
	while(first != last){
		--last;
		if(pred(*last))
			break;
	}

	return last;
}

inline bool isnotwspace(wchar_t c)
{
	return !iswspace(c);
}

inline bool isbackslash(wchar_t c)
{
	return c == L'\\' || c == L'/';
}

bool readable(const wstring &filename)
{
	FILE
		*fp = _wfopen(filename.c_str(), L"rb");

	if(fp != NULL){
		fclose(fp);
		return true;
	}

	return false;
}

inline bool executable(const wstring &filename)
{
	return readable(filename);
}

////////////////////////////////////////////////////////////////////////

class String : public wstring {
	typedef String Self;
	typedef wstring Super;

public:
	String();
	String(const Super &s);
	String(const wchar_t *s);
	String(const_iterator b, const_iterator e);
	String(const char *s);
	~String();

	string to_ansi() const;
	Self to_upper() const;
	Self trim() const;
	bool isdoublequote() const;
	Self doublequote() const;
	Self doublequote_del() const;

	Self &operator=(const Self &s);
	Self &operator=(const wchar_t *s);
	Self &operator=(const char *s);

	Self &operator+=(const Self &s)				{ append(s); return *this; }
	Self &operator+=(const wchar_t *s)			{ append(s); return *this; }
	Self &operator+=(const char *s);

	Self &assign_from_ansi(const char *s);
	Self &assign_from_ansi(const string &s)		{ return assign_from_ansi(s.c_str()); }
	Self &assign_from_utf8(const char *s);
	Self &assign_from_utf8(const string &s)		{ return assign_from_utf8(s.c_str()); }
	Self &assign_from_env(const Self &name);
	Self &printf(Self format, ...);

	// filename operator
	bool have_path() const;
	bool have_ext() const;
	bool isbackslash() const;
	Self backslash() const;
	Self subext(const Self &ext) const;
	Self drivename() const;
	Self dirname() const;
	Self basename() const;
};

String::String()									{}
String::String(const String::Super &s)				: Super(s) {}
String::String(const wchar_t *s)					: Super(s) {}
String::String(const_iterator b, const_iterator e)	: Super(b, e) {}
String::String(const char *s)						{ assign_from_ansi(s); }
String::~String()									{}
String &String::operator=(const String &s)			{ assign(s); return *this; }
String &String::operator=(const wchar_t *s)			{ assign(s); return *this; }
String &String::operator=(const char *s)			{ assign_from_ansi(s); return *this; }
String &String::operator+=(const char *s)			{ append(String(s)); return *this; }
String operator+(const String &s1, const char *s2)	{ return s1 + String(s2); }
String operator+(const char *s1, const String &s2)	{ return String(s1) + s2; }
bool operator==(const String &s1, const char *s2)	{ return s1 == String(s2); }
bool operator==(const char *s1, const String &s2)	{ return String(s1) == s2; }

string String::to_ansi() const
{
	sint
		siz = WideCharToMultiByte(CP_ACP, 0, c_str(), size(), NULL, 0, NULL, NULL);
	char
		*buf = new char[siz+1];

	fill(buf, buf + siz+1, 0);

	WideCharToMultiByte(CP_ACP, 0, c_str(), size(), buf, siz, NULL, NULL);

	string
		r(buf);

	delete [] buf;

	return r;
}

String String::trim() const
{
	const_iterator
		b = begin(),
		e = end();

	b = ::find_if(b, e, isnotwspace);
	e = ::rfind_if(b, e, isnotwspace);

	if(e != end() && isnotwspace(*e))
		++e;

	return Self(b, e);
}

bool String::isdoublequote() const
{
	// BUG:
	//  �P�ɐ擪�ƍŌ�̕����� " ���ǂ����𔻒肵�Ă��邾���Ȃ̂ŁA������̓r���� " ���������ꍇ�Ȃǂ��l�����Ă��Ȃ��B

	if(size() >= 2 && *begin() == L'"' && *(end()-1) == L'"')
		return true;

	return false;
}

String String::doublequote_del() const
{
	if(isdoublequote())
		return String(begin()+1, end()-1);
	else
		return String(*this);
}

String &String::assign_from_ansi(const char *s)
{
	sint
		size = MultiByteToWideChar(CP_ACP, 0, s, -1, NULL, 0);
	wchar_t
		*buf = new wchar_t[size+1];

	fill(buf, buf + size+1, 0);

	MultiByteToWideChar(CP_ACP, 0, s, -1, buf, size);

	assign(buf);

	delete [] buf;

	return *this;
}

String &String::printf(String format, ...) // va���g���K�v��AString�͒l�n��
{
	wchar_t
		buf[1024+1];
	va_list
		va;
	sint
		size;

	va_start(va, format);
	size = wvsprintf(buf, format.c_str(), va);
	va_end(va);

	assign(buf, buf + size);

	return *this;
}

bool String::isbackslash() const
{
	return size() > 0 && ::isbackslash(*(end()-1));
}

String String::dirname() const
{
	const_iterator
		e = ::rfind_if(begin(), end(), ::isbackslash);

	if(e != end() && ::isbackslash(*e))
		++e;

	return Self(begin(), e);
}

////////////////////////////////////////////////////////////////////////

void _putcxxx(const char *s, FILE *fp)
{
	fputs(s, fp);
	fputc('\n', fp);
}

inline void putcout(const char *s)
{
	_putcxxx(s, stdout);
}

inline void putcerr(const char *s)
{
	_putcxxx(s, stderr);
}

void _putcxxx(const String &s, FILE *fp)
{
	_putcxxx(s.to_ansi().c_str(), fp);
}

inline void putcout(const String &s)
{
	_putcxxx(s, stdout);
}

inline void putcerr(const String &s)
{
	_putcxxx(s, stderr);
}

void _putcxxx(const char *s1, const String &s2, FILE *fp)
{
	_putcxxx(String().assign_from_ansi(s1) + s2, fp);
}

inline void putcout(const char *s1, const String &s2)
{
	_putcxxx(s1, s2, stdout);
}

inline void putcerr(const char *s1, const String &s2)
{
	_putcxxx(s1, s2, stderr);
}

////////////////////////////////////////////////////////////////////////

class WindowsAPI {
public:
	static bool CreateProcess(const String &cmd, DWORD CreationFlags, const String &wd, LPSTARTUPINFO si, LPPROCESS_INFORMATION pi);
	static String GetClipboardText();
	static String GetCommandLine()		{ return ::GetCommandLine(); }
	static String GetComputerName();
	static String GetModuleFileName(HMODULE Module = 0);
	static String GetTempPath();
	static String GetUserName();
	static String SHGetSpecialFolder(sint nFolder);
};

////////////////////////////////////////////////////////////////////////

String get_command_name(const String &cmd)
{
	bool
		in_quote = false;

	for(String::const_iterator i = cmd.begin() ; i != cmd.end() ; ++i){
		wchar_t
			c = *i;

		if(!in_quote)
			if(iswspace(c))
				return String(cmd.begin(), i).doublequote_del();

		if(c == L'"')
			in_quote = in_quote ? false : true;
	}

	return cmd.doublequote_del();
}

String get_given_option(const String &cmd)
{
	bool
		in_quote = false;

	for(String::const_iterator i = cmd.begin() ; i != cmd.end() ; ++i){
		wchar_t
			c = *i;

		if(!in_quote)
			if(iswspace(c))
				return String(i, cmd.end());

		if(c == L'"')
			in_quote = in_quote ? false : true;
	}

	return String();
}

bool CreateDaemonProcess(const String &cmd)
{
	STARTUPINFO
		si;
	PROCESS_INFORMATION
		pi;
	DWORD
		CreationFlags = 0;
	wchar_t
		*cmd_c_str;
	String
		wd(get_command_name(cmd).dirname());
	bool
		r;

	memset(&si, 0, sizeof si);
	si.cb = sizeof si;
	memset(&pi, 0, sizeof pi);

	cmd_c_str = new wchar_t[cmd.size()+1];
	copy(cmd.begin(), cmd.end(), cmd_c_str);
	cmd_c_str[cmd.size()] = 0;

//	CreationFlags |= CREATE_BREAKAWAY_FROM_JOB;
	CreationFlags |= CREATE_DEFAULT_ERROR_MODE;
	CreationFlags |= CREATE_NEW_PROCESS_GROUP;
	CreationFlags |= CREATE_NO_WINDOW;

	si.wShowWindow = SW_SHOWMINIMIZED;
	si.dwFlags |= STARTF_USESHOWWINDOW;
	si.dwFlags |= STARTF_FORCEOFFFEEDBACK;

	r = ::CreateProcess(NULL, cmd_c_str, NULL, NULL, FALSE, CreationFlags, NULL, (wd.size()>0 ? wd.c_str() : NULL), &si, &pi);

	delete [] cmd_c_str;

	return r;
}

void StartDaemon_main()
{
	String
		cmdline,
		cmd;

	// �R�}���h���C���𓾂�
	cmdline = get_given_option(WindowsAPI::GetCommandLine()).trim();
	cmd = get_command_name(cmdline);

	// �p�����[�^���Ȃ��ꍇ�A�w���v��\��
	if(cmd.size() <= 0){
		putcout(credit);
		putcout("");
		putcout("Usage: " PGM " [command] [option...]");
		error = true; // �����ɂ̓G���[�ł͂Ȃ�
		rcode = 1;
		return;
	}

	// �R�}���h���t���p�X�\�L�łȂ��ꍇ�A�G���[
	if(!iswalpha(cmd[0]) || cmd[1] != L':' || cmd[2] != L'\\'){
		putcerr(PGM_ERR "command name must be full path form: ", cmd);
		error = true;
		rcode = 1;
		return;
	}

	// �R�}���h�����݂��Ȃ��ꍇ�A�G���[
	if(!executable(cmd)){
		putcerr(PGM_ERR "command not found: ", cmd);
		error = true;
		rcode = 1;
		return;
	}

	// �v���Z�X�N��
	if(CreateDaemonProcess(cmdline) == 0){
		putcerr(PGM_ERR "CreateProcess() failed: ", cmdline);
		error = true;
		rcode = 1;
	}
}

sint wmain(sint, wchar_t *[])
{
	StartDaemon_main();

	return rcode;
}

