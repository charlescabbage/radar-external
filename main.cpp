#include <windows.h>
#include <sstream>
#include "HackProcess.h"
#include "ProcMem.h"

#define WPN_MELEE	0xAC
#define WPN_RIFLE   0xBC
#define WPN_SHOTGUN 0xCC
#define WPN_SNIPER  0xDC
#define WPN_GATLING 0xEC
#define WPN_BAZOOKA 0xFC
#define WPN_GRENADE 0xC

std::string strWeapon[] = { "", "Melee", "Rifle", "Shotgun", "Sniper", "Gatling", "Bazooka", "Grenade" };

CHackProcess fProcess;
ProcMem Mem;

int NumOfPlayers = 16;
DWORD PlayerCountOffset[] = { 0xD4 };

DWORD PlayerBase = 0x01195944;
DWORD EntityPlayer_Base = PlayerBase + 0x4;
DWORD EntityLoopDistance = 0x4;

DWORD InfoPlayerOffsets[] = { 0x7C, 0x10C, 0x4, 0x4 };
DWORD PosPlayerOffsets[] = { 0x7C, 0x124, 0x40C, 0x54 };

DWORD RotationBase = 0x01199950;
DWORD RotationOffset[] = { 0xC };

HWND TargetWnd;
HDC HDC_Desktop;
RECT m_Rect;

HFONT Font;

HBRUSH EnemyBrush;
HBRUSH TeammateBrush;
HBRUSH MyPlayerBrush;

COLORREF TextCOLOR;

int zoom = 12;

int Screen_Width;
int Screen_Height;


LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
	switch (Message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_MOUSEWHEEL:
		if ((short)HIWORD(wParam) > 0)
		{
			if (zoom != 48)
				zoom++;
		}
		else
		{
			if (zoom != 12)
				zoom--;
		}
		break;
	default:
		return DefWindowProc(hwnd, Message, wParam, lParam);
	}
	return 0;
}


void UpdateFont()
{
	Font = CreateFont(
		zoom,								// nHeight
		(zoom / 2) - (zoom / 2 / 2 - 1),    // nWidth
		0,									// nEscapement
		0,									// nOrientation
		FW_EXTRABOLD,						// nWeight
		FALSE,								// bItalic
		FALSE,								// bUnderline
		0,									// cStrikeOut
		ANSI_CHARSET,						// nCharSet
		OUT_DEFAULT_PRECIS,					// nOutPrecision
		CLIP_DEFAULT_PRECIS,				// nClipPrecision
		DEFAULT_QUALITY,					// nQuality
		DEFAULT_PITCH | FF_SWISS,			// nPitchAndFamily
		"Calibri");
}


void SetupDrawing(HWND handle)
{
	TargetWnd = handle;
	HDC_Desktop = GetDC(handle);
	EnemyBrush = CreateSolidBrush(RGB(255, 0, 0));
	TeammateBrush = CreateSolidBrush(RGB(0, 255, 0));
	MyPlayerBrush = CreateSolidBrush(RGB(0, 102, 204));
	TextCOLOR = RGB(255, 255, 255);
	UpdateFont();
}


typedef struct
{
	float x;
	float y;
}Vector2D;


typedef struct
{
	unsigned char __idk;
	unsigned char Team;
	unsigned char __idk2;
	unsigned char __idk3;
	unsigned char State;
	unsigned char __idk4;
	unsigned char __idk5;
	unsigned char __idk6;
	int Health;
	int __idk7;
	int HealthKey;
}PlayerDataVec;


int WeaponIndex(unsigned char state)
{
	switch (state)
	{
	case WPN_MELEE:
		return 1;
		break;
	case WPN_RIFLE:
		return 2;
		break;
	case WPN_SHOTGUN:
		return 3;
		break;
	case WPN_SNIPER:
		return 4;
		break;
	case WPN_GATLING:
		return 5;
		break;
	case WPN_BAZOOKA:
		return 6;
		break;
	case WPN_GRENADE:
		return 7;
		break;
	}
	return 0;
}


bool getBit(unsigned char byte, int position)
{
	return (byte >> position) & 0x1;
}


struct MyPlayer_t
{
	Vector2D Position;
	PlayerDataVec info;
	bool Team;
	int Weapon;
	int Health;
	bool isDead;
	float Rotation;

	void ReadInformation()
	{
		Position = Mem.Read<Vector2D>(fProcess.__hProcess, PlayerBase, PosPlayerOffsets, 4);

		info = Mem.Read<PlayerDataVec>(fProcess.__hProcess, PlayerBase, InfoPlayerOffsets, 4);

		Team = getBit(info.Team, 4);

		Weapon = WeaponIndex(info.State);

		Health = (info.Health ^ info.HealthKey);

		isDead = getBit(info.State, 0) || !Health || !Weapon || (!Position.x && !Position.y);

		Rotation = Mem.Read<float>(fProcess.__hProcess, RotationBase, RotationOffset, 1);

		NumOfPlayers = Mem.Read<int>(fProcess.__hProcess, EntityPlayer_Base, PlayerCountOffset, 1);
	}

}MyPlayer;


struct PlayerList_t
{
	Vector2D Position;
	PlayerDataVec info;
	bool Team;
	int Weapon;
	int Health;
	bool isDead;
	
	void ReadInformation(int player)
	{
		DWORD EntityOffset = (player * EntityLoopDistance) + 0x54;

		DWORD InfoEntityOffsets[] = { EntityOffset, 0x4, 0x4 };

		DWORD PosEntityOffsets[] = { EntityOffset, 0x58, 0x124, 0x40C, 0x54 };

		Position = Mem.Read<Vector2D>(fProcess.__hProcess, EntityPlayer_Base, PosEntityOffsets, 5);

		info = Mem.Read<PlayerDataVec>(fProcess.__hProcess, EntityPlayer_Base, InfoEntityOffsets, 3);

		Team = getBit(info.Team, 4);

		Weapon = WeaponIndex(info.State);

		Health = (info.Health ^ info.HealthKey);

		isDead = getBit(info.State, 0) || !Health || !Weapon || (!Position.x && !Position.y);
	}

}PlayerList[16];


void WorldToRadar(Vector2D WorldLocation, Vector2D mypos, float yaw, int width, int height, int radius, Vector2D &pos)
{
	float cosYaw = cos(-yaw);
	float sinYaw = sin(-yaw);

	float distX = WorldLocation.x - mypos.x;
	float distY = WorldLocation.y - mypos.y;

	pos.x = ((distY * cosYaw - distX * sinYaw) * -1 / radius) + (width / 2 - 5);
	pos.y = ((distX * cosYaw + distY * sinYaw) / radius) * -1 + (height / 2 - 5);
}


void DrawFilledRect(HBRUSH hbr, int x, int y, int w, int h)
{
	RECT rect = { x, y, x + w, y + h };
	FillRect(HDC_Desktop, &rect, hbr);
}


void DrawString(int x, int y, COLORREF color, const char* text)
{
	SetTextAlign(HDC_Desktop, TA_CENTER | TA_NOUPDATECP);
	SetBkColor(HDC_Desktop, RGB(0, 0, 0));
	SetBkMode(HDC_Desktop, TRANSPARENT);
	SetTextColor(HDC_Desktop, color);
	SelectObject(HDC_Desktop, Font);
	TextOutA(HDC_Desktop, x, y, text, strlen(text));
	DeleteObject(Font);
}


void DrawRadar(int x, int y, int w, int h, bool isEnemy, int health, int weapon)
{
	if (isEnemy)
		DrawFilledRect(EnemyBrush, x, y, w, h);
	else
		DrawFilledRect(TeammateBrush, x, y, w, h);

	std::stringstream ss;
	ss << health;
	DrawString(x + (w / 2), y + h, TextCOLOR, strWeapon[weapon].c_str());
	DrawString(x + (w / 2), y + (h * 2), TextCOLOR, ss.str().c_str());
}


void Radar()
{
	GetWindowRect(TargetWnd, &m_Rect);
	Screen_Width = (int)(m_Rect.right - m_Rect.left);
	Screen_Height = (int)(m_Rect.bottom - m_Rect.top);

	DrawFilledRect(MyPlayerBrush, Screen_Width / 2 - 5, Screen_Height / 2 - 5, zoom, zoom);
	InvalidateRect(TargetWnd, NULL, TRUE);
	DrawFilledRect(MyPlayerBrush, Screen_Width / 2 - 5, Screen_Height / 2 - 5, zoom, zoom);

	for (int i = 0; i < NumOfPlayers; i++)
	{
		if (MyPlayer.isDead)
			break;

		PlayerList[i].ReadInformation(i);

		if (PlayerList[i].isDead)
			continue;

		if (PlayerList[i].Team == MyPlayer.Team &&
			PlayerList[i].Health == MyPlayer.Health &&
			PlayerList[i].Weapon == MyPlayer.Weapon)
			continue;

		Vector2D EntityPos;
		bool isEnemy = (MyPlayer.Team != PlayerList[i].Team);
		WorldToRadar(PlayerList[i].Position, MyPlayer.Position, MyPlayer.Rotation, Screen_Width, Screen_Height, zoom, EntityPos);
		DrawRadar((int)EntityPos.x, (int)EntityPos.y, zoom, zoom, isEnemy, PlayerList[i].Health, PlayerList[i].Weapon);
	}

	Sleep(25);
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{

	if (!fProcess.RunProcess())
	{
		MessageBox(NULL, "Game not found!", "Error", MB_ICONERROR | MB_OK);
		return 0;
	}

#pragma region WINDOW CLASS

	WNDCLASSEX wc;
	HWND hwnd;
	MSG msg;

	memset(&wc, 0, sizeof(wc));
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);

	wc.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
	wc.lpszClassName = "WindowClass";
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	if (!RegisterClassEx(&wc))
	{
		MessageBox(NULL, "Window Registration Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	hwnd = CreateWindowEx(WS_EX_CLIENTEDGE | WS_EX_TOPMOST, "WindowClass", "Radar", WS_VISIBLE | WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		416,
		438,
		NULL, NULL, hInstance, NULL);

	if (hwnd == NULL)
	{
		MessageBox(NULL, "Window Creation Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

#pragma endregion

	SetupDrawing(hwnd);

	while (GetMessage(&msg, NULL, 0, 0) > 0 && fProcess.ProcessRunning("Game.exe"))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);

		UpdateFont();

		MyPlayer.ReadInformation();

		Radar();
	}

	return msg.wParam;
}