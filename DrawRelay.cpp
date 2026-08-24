////////////////////////////////////////////////////////////////////////////////
//
//  DrawRelay.cpp
//
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  ヘッダファイル
//

#include <Windows.h>
#include <WinSock.h>
#include <stdio.h>
#include <time.h>
////////////////////////////////////////////////////////////////////////////////
//
// 使用ライブラリ
//
#pragma comment(lib, "wsock32.lib")

////////////////////////////////////////////////////////////////////////////////
//
//  定数定義
//
#define WM_SOCKET (WM_USER + 1) // ソケット用メッセージ
#define PORT 10000							// 通信ポート番号

#define IDB_CONNECT 1000	// [接続]ボタン
#define IDB_ACCEPT 1001		// [接続待ち]ボタン
#define IDB_REJECT 1002		// [切断]ボタン
#define IDB_SEND 1003			// [切断要請]ボタン
#define IDB_ANSWER 1004		// [回答]ボタン  //編集
#define IDB_SKIP 1005			// [スキップ]ボタン //// 追加 1/8
#define IDB_BLACK 1100		// [黒]ボタン (追加1/4)
#define IDB_RED 1101			// [赤]ボタン (追加1/4)
#define IDB_GRAY 1102			// [消しゴム]ボタン (追加1/10)
#define IDB_BROWN 1103		// [茶]ボタン
#define IDB_YELLOW 1104		// [黄]ボタン
#define IDB_BLUE 1105			// [青]ボタン
#define IDB_GREEN 1106		// [緑]ボタン (ここまで1/10)
#define IDB_ALLCLEAR 1107 // [全消し]ボタン 1/23

#define IDF_HOSTNAME 2000 // ホスト名入力エディットボックス
#define IDF_ANSMSG 2001		// 回答エディットボックス
#define IDF_RIGHTMSG 2002 // 正誤判定エディットボックス
#define IDF_ANSWER 2003		// 回答入力エディットボックス
#define IDF_ODAI 2004			// お題表示用エディットボックス(追加)
#define IDF_POSITION 2005 // 自分がお絵描き側か回答側かを表示するエディットボックス //// 追加 1/10

#define IDE_RECVMSG 3000 // メッセージ受信イベント

#define WINDOW_W 1200 // ウィンドウの幅
#define WINDOW_H 750	// ウィンドウの高さ

#define MAX_MESSAGE 20	// 配列の最大要素数
#define MAX_ARRAY 10000 // 配列の最大要素数
#define BLACK 4000			// 黒ペン識別子 (追加1/4)
#define RED 4001				// 赤ペン識別子 (追加1/4)
#define GRAY 4002				// 灰色ペン(消しゴム用)識別子 (追加1/10)
#define BROWN 4003			// 茶色ペン識別子
#define YELLOW 4004			// 黄色ペン識別子
#define BLUE 4005				// 青色ペン識別子
#define GREEN 4006			// 緑ペン識別子 (ここまで1/10)

////////////////////////////////////////////////////////////////////////////////
//
//  グローバル変数
//
LPCTSTR lpClassName = "tegakiChat";			 // ウィンドウクラス名
LPCTSTR lpWindowName = "手描きチャット"; // タイトルバーにつく名前

SOCKET sock = INVALID_SOCKET;		 // ソケット
SOCKET sv_sock = INVALID_SOCKET; // サーバ用ソケット
HOSTENT *phe;										 // HOSTENT構造体

HPEN hPenBlack;	 // 黒ペン
HPEN hPenRed;		 // 赤ペン
HPEN hPenGray;	 // 灰色ペン(消しゴム用) (追加1/10)
HPEN hPenBrown;	 // 茶色ペン
HPEN hPenYellow; // 黄色ペン
HPEN hPenBlue;	 // 青ペン
HPEN hPenGreen;	 // 緑ペン (ここまで1/10)

const RECT d = {10, 90, 810, 690}; // 描画領域(左上隅のx座標, 左上隅のy座標, 右下隅のx座標, 右下隅のy座標)

int n;											 // カウンタ(自分用)
int n2;											 // カウンタ(相手用)
int flag[MAX_ARRAY];				 // ペンダウンフラグ(自分用)
int flag2[MAX_ARRAY];				 // ペンダウンフラグ(相手用)
int point_color[MAX_ARRAY];	 // 色識別子(自分用) //(追加1/15)
int point_color2[MAX_ARRAY]; // 色識別子(相手用)
POINT pos[MAX_ARRAY];				 // 座標を格納(自分用)
POINT pos2[MAX_ARRAY];			 // 座標を格納(相手用)
int active = 0;
int ansflag = 0;		// 回答者決めるフラグ(追加)1=回答者 2=お絵かき側
char subject[20];		// 正解のお題(追加)
int randval;				//(追加)12/20
int color = BLACK;	// 色を格納(自分用)(デフォルトは黒)(追加 1/4)
int color2 = BLACK; // 色を格納(相手用)(デフォルトは黒)(追加 1/4)
////////////////////////////////////////////////////////////////////////////////
//
//  プロトタイプ宣言
//
LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM); // ウィンドウ関数
LRESULT CALLBACK OnPaint(HWND, UINT, WPARAM, LPARAM);		 // 描画関数

BOOL SockInit(HWND hWnd);									// ソケット初期化
BOOL SockAccept(HWND hWnd);								// ソケット接続待ち
BOOL SockConnect(HWND hWnd, LPCSTR host); // ソケット接続
BOOL checkMousePos(int x, int y);					// マウスの位置がキャンパスの中かどうか判定する
void setData(int flag, int x, int y);			// 描画情報を入れる
void setData2(int flag, int x, int y);		// 描画情報を入れる
void resetData2();												// 描写情報の初期化(追加)12/20
void resetData();													// 描写情報の初期化(追加)12/20

////////////////////////////////////////////////////////////////////////////////
//
//  WinMain関数 (Windowsプログラム起動時に呼ばれる関数)
//
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	HWND hWnd;		 // ウィンドウハンドル
	MSG msg;			 // メッセージ
	WNDCLASSEX wc; // ウィンドウクラス

	// ウィンドウクラス定義
	wc.hInstance = hInstance;										// インスタンス
	wc.lpszClassName = lpClassName;							// クラス名
	wc.lpfnWndProc = WindowProc;								// ウィンドウ関数名
	wc.style = 0;																// クラススタイル
	wc.cbSize = sizeof(WNDCLASSEX);							// 構造体サイズ
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION); // アイコンハンドル
	wc.hIconSm = LoadIcon(NULL, IDI_WINLOGO);		// スモールアイコン
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);		// マウスポインタ
	wc.lpszMenuName = NULL;											// メニュー(なし)
	wc.cbClsExtra = 0;													// クラス拡張情報
	wc.cbWndExtra = 0;													// ウィンドウ拡張情報
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;		// ウィンドウの背景色
	if (!RegisterClassEx(&wc))
		return 0; // ウィンドウクラス登録

	// ウィンドウ生成
	hWnd = CreateWindow(
			lpClassName,													 // ウィンドウクラス名
			lpWindowName,													 // ウィンドウ名
			WS_DLGFRAME | WS_VISIBLE | WS_SYSMENU, // ウィンドウ属性
			CW_USEDEFAULT,												 // ウィンドウ表示位置(X)
			CW_USEDEFAULT,												 // ウィンドウ表示位置(Y)
			WINDOW_W,															 // ウィンドウサイズ(X)
			WINDOW_H,															 // ウィンドウサイズ(Y)
			HWND_DESKTOP,													 // 親ウィンドウハンドル
			NULL,
			hInstance, // インスタンスハンドル
			NULL);

	// ウィンドウ表示
	ShowWindow(hWnd, nCmdShow); // ウィンドウ表示モード
	UpdateWindow(hWnd);					// ウインドウ更新

	// メッセージループ
	while (GetMessage(&msg, NULL, 0, 0))
	{ // メッセージを取得
		TranslateMessage(&msg);
		DispatchMessage(&msg); // メッセージ送る
	}
	return (int)msg.wParam; // プログラム終了
}

////////////////////////////////////////////////////////////////////////////////
//
//  ウィンドウ関数(イベント処理を記述)（追加あり）
//
LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wP, LPARAM lP)
{
	static HWND hWndHost;								 // ホスト名入力用エディットボックス
	static HWND hWndConnect, hWndAccept; // [接続]ボタンと[接続待ち]ボタン
	static HWND hWndReject;							 // [切断]ボタン
	static HWND hWndSend;								 // [切断要請]ボタン
	static BOOL mouseFlg = FALSE;				 // 前回の状態 TRUE:描画した、FALSE:描画していない
	static HWND hWndSendANS;						 // 回答入力用エディットボックス  //編集開始
	static HWND hWndAnswer;							 // [回答]ボタン   //編集終
	static HWND hWndAnsMSG;							 // 回答表示用エディットボックス(追加)
	static HWND hWndRightMSG;						 // 正誤表示用エディットボックス(追加)
	static HWND hWndSubMSG;							 // お題用エディットボックス(追加)
	static HWND hWndBlack;							 // [黒]ボタン (追加1/4)
	static HWND hWndRed;								 // [赤]ボタン (追加1/4)
	static HWND hWndGray;								 // [消しゴム]ボタン(灰色) (追加1/10)
	static HWND hWndBrown;							 // [茶]ボタン
	static HWND hWndYellow;							 // [黄]ボタン
	static HWND hWndBlue;								 // [青]ボタン
	static HWND hWndGreen;							 // [緑]ボタン (ここまで1/10)
	static HWND hWndAllClear;						 // [全消し]ボタン 1/23
	static HWND hWndSkip;								 // [スキップ]ボタン //// 追加 1/8
	static HWND hWndPositionMSG;				 // 自分がお絵描き側か回答側かを表示するエディットボックス //// 追加 1/10

	switch (uMsg)
	{
	case WM_CREATE:																								 // ウィンドウが生成された
		hPenBlack = (HPEN)CreatePen(PS_SOLID, 3, RGB(0, 0, 0));			 // 黒ペンの生成
		hPenRed = (HPEN)CreatePen(PS_SOLID, 3, RGB(255, 0, 0));			 // 赤ペンの生成
		hPenGray = (HPEN)CreatePen(PS_SOLID, 3, RGB(240, 240, 240)); // 灰色ペンの生成(消しゴム用) (追加 1/10)
		hPenBrown = (HPEN)CreatePen(PS_SOLID, 3, RGB(128, 0, 0));		 // 茶色ペンの生成
		hPenYellow = (HPEN)CreatePen(PS_SOLID, 3, RGB(255, 255, 0)); // 黄色ペンの生成
		hPenBlue = (HPEN)CreatePen(PS_SOLID, 3, RGB(0, 0, 255));		 // 青ペンの生成
		hPenGreen = (HPEN)CreatePen(PS_SOLID, 3, RGB(0, 128, 0));		 // 緑ペンの生成 (ここまで1/10)
		// 文字列表示
		CreateWindow("static", "Host Name",
								 WS_CHILD | WS_VISIBLE, 10, 10, 100, 18,
								 hWnd, NULL, NULL, NULL);
		CreateWindow("static", "回答", //(追加)
								 WS_CHILD | WS_VISIBLE, 900, 70, 100, 18,
								 hWnd, NULL, NULL, NULL);
		CreateWindow("static", "正誤", // （追加）
								 WS_CHILD | WS_VISIBLE, 900, 220, 100, 18,
								 hWnd, NULL, NULL, NULL);
		// ホスト名入力用エディットボックス
		hWndHost = CreateWindowEx(WS_EX_CLIENTEDGE, "edit", "",
															WS_CHILD | WS_VISIBLE, 10, 30, 200, 25,
															hWnd, (HMENU)IDF_HOSTNAME, NULL, NULL);
		// [接続]ボタン
		hWndConnect = CreateWindow("button", "接続",
															 WS_CHILD | WS_VISIBLE, 220, 30, 50, 25,
															 hWnd, (HMENU)IDB_CONNECT, NULL, NULL);
		// [接続待ち]ボタン
		hWndAccept = CreateWindow("button", "接続待ち",
															WS_CHILD | WS_VISIBLE, 275, 30, 90, 25,
															hWnd, (HMENU)IDB_ACCEPT, NULL, NULL);
		// [切断要請]ボタン
		hWndSend = CreateWindow("button", "切断要請",
														WS_CHILD | WS_VISIBLE | WS_DISABLED, 220, 60, 90, 25,
														hWnd, (HMENU)IDB_SEND, NULL, NULL);
		// [切断]ボタン
		hWndReject = CreateWindow("button", "切断",
															WS_CHILD | WS_VISIBLE | WS_DISABLED, 315, 60, 50, 25,
															hWnd, (HMENU)IDB_REJECT, NULL, NULL);
		// [スキップ]ボタン //// 追加 1/8
		hWndSkip = CreateWindow("button", "スキップ",
														WS_CHILD | WS_VISIBLE | WS_DISABLED, 880, 30, 70, 25,
														hWnd, (HMENU)IDB_SKIP, NULL, NULL);
		// 回答表示用エディットボックス(追加)
		hWndAnsMSG = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("edit"), TEXT(""),
																WS_CHILD | WS_VISIBLE | ES_MULTILINE | WS_DISABLED, 900, 100, 200, 30,
																hWnd, (HMENU)IDF_ANSMSG, NULL, NULL);
		// 正誤表示用エディットボックス(追加)
		hWndRightMSG = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("edit"), TEXT(""),
																	WS_CHILD | WS_VISIBLE | ES_MULTILINE | WS_DISABLED, 900, 250, 200, 30,
																	hWnd, (HMENU)IDF_ANSMSG, NULL, NULL);
		// 文字列表示 //編集開始 //編集開始(12/20)
		CreateWindow("static", "回答欄",
								 WS_CHILD | WS_VISIBLE, 930, 300, 100, 18,
								 hWnd, NULL, NULL, NULL);

		// 回答入力用エディットボックス
		hWndSendANS = CreateWindowEx(WS_EX_CLIENTEDGE, "edit", "",
																 WS_CHILD | WS_VISIBLE | ES_MULTILINE | WS_DISABLED, 930, 330, 150, 25,
																 hWnd, (HMENU)IDF_ANSWER, NULL, NULL);
		// [回答]ボタン
		hWndAnswer = CreateWindow("button", "回答",
															WS_CHILD | WS_VISIBLE | WS_DISABLED, 980, 360, 50, 25,
															hWnd, (HMENU)IDB_ANSWER, NULL, NULL); // 編集終 //編集終(12/20)
		////////////////追加////////////////
		// 文字列表示
		CreateWindow("static", "お題",
								 WS_CHILD | WS_VISIBLE, 500, 10, 100, 18,
								 hWnd, NULL, NULL, NULL);
		// お題表示テキストボックス
		hWndSubMSG = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("edit"), TEXT(""),
																WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY, 500, 40, 250, 25,
																hWnd, (HMENU)IDF_ODAI, NULL, NULL);
		/////////ここまで///////////////////////////////
		// [黒]ボタン (追加 1/4)
		hWndBlack = CreateWindow("button", "黒",
														 WS_CHILD | WS_VISIBLE | WS_DISABLED, 980, 400, 50, 25,
														 hWnd, (HMENU)IDB_BLACK, NULL, NULL); // (ここまで 1/4)
		// [赤]ボタン (追加 1/4)
		hWndRed = CreateWindow("button", "赤",
													 WS_CHILD | WS_VISIBLE | WS_DISABLED, 980, 430, 50, 25,
													 hWnd, (HMENU)IDB_RED, NULL, NULL); // (ここまで 1/4)
																															// [消しゴム]ボタン (追加 1/10)
		hWndGray = CreateWindow("button", "消しゴム",
														WS_CHILD | WS_VISIBLE | WS_DISABLED, 980, 460, 100, 25,
														hWnd, (HMENU)IDB_GRAY, NULL, NULL); //(ここまで 1/10)
		// [茶]ボタン (追加 1/10)
		hWndBrown = CreateWindow("button", "茶",
														 WS_CHILD | WS_VISIBLE | WS_DISABLED, 980, 490, 50, 25,
														 hWnd, (HMENU)IDB_BROWN, NULL, NULL); // (ここまで 1/10)
		// [黄]ボタン (追加 1/10)
		hWndYellow = CreateWindow("button", "黄",
															WS_CHILD | WS_VISIBLE | WS_DISABLED, 980, 520, 50, 25,
															hWnd, (HMENU)IDB_YELLOW, NULL, NULL); // (ここまで 1/10)
		// [青]ボタン (追加 1/10)
		hWndBlue = CreateWindow("button", "青",
														WS_CHILD | WS_VISIBLE | WS_DISABLED, 980, 550, 50, 25,
														hWnd, (HMENU)IDB_BLUE, NULL, NULL); // (ここまで 1/10)
		// [緑]ボタン (追加 1/10)
		hWndGreen = CreateWindow("button", "緑",
														 WS_CHILD | WS_VISIBLE | WS_DISABLED, 980, 580, 50, 25,
														 hWnd, (HMENU)IDB_GREEN, NULL, NULL); // (ここまで 1/10)
																																	////////////////追加 1/10/////////////////////////////
		// 自分がお絵描き側か回答側かを表示するエディットボックス1/10追加
		hWndPositionMSG = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("edit"), TEXT(""),
																		 WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY, 900, 620, 200, 25,
																		 hWnd, (HMENU)IDF_POSITION, NULL, NULL);
		// [全消し]ボタン (追加 1/23)
		hWndAllClear = CreateWindow("button", "全消し",
																WS_CHILD | WS_VISIBLE | WS_DISABLED, 980, 650, 55, 20,
																hWnd, (HMENU)IDB_ALLCLEAR, NULL, NULL); // (ここまで 1/10)

		SetFocus(hWndHost); // フォーカス指定
		SockInit(hWnd);			// ソケット初期化

		return 0L;

	case WM_COMMAND: // ボタンが押された
		switch (LOWORD(wP))
		{
		case IDB_ACCEPT: // [接続待ち]ボタン押下(サーバー)
			if (SockAccept(hWnd))
			{						 // 接続待ち要求
				return 0L; // 接続待ち失敗
			}
			EnableWindow(hWndHost, FALSE);		 // [HostName]無効
			EnableWindow(hWndConnect, FALSE);	 // [接続]    無効
			EnableWindow(hWndAccept, FALSE);	 // [接続待ち]無効
			EnableWindow(hWndReject, TRUE);		 // [切断]    有効
			EnableWindow(hWndSendANS, FALSE);	 // [回答欄]  無効   //編集開始
			EnableWindow(hWndAnswer, FALSE);	 // [回答]    無効   //編集終
			EnableWindow(hWndBlack, FALSE);		 // [黒]      無効 (追加1/4)
			EnableWindow(hWndRed, FALSE);			 // [赤]      無効 (追加1/4)
			EnableWindow(hWndGray, FALSE);		 // [消しゴム]無効 (追加1/10)
			EnableWindow(hWndBrown, FALSE);		 // [茶]      無効
			EnableWindow(hWndYellow, FALSE);	 // [黄]      無効
			EnableWindow(hWndBlue, FALSE);		 // [青]      無効
			EnableWindow(hWndGreen, FALSE);		 // [緑]      無効 (ここまで1/10)
			EnableWindow(hWndAllClear, FALSE); // [全消し]      無効 1/23
			EnableWindow(hWndSkip, FALSE);		 // [スキップ]  無効  //// 追加 1/8
			ansflag = 2;											 /////////////////////////////お絵かき側（追加）
			return 0L;

		case IDB_CONNECT: // [接続]ボタン押下(クライアント)
			char host[100];
			GetWindowText(hWndHost, host, sizeof(host));

			if (SockConnect(hWnd, host))
			{											// 接続要求
				SetFocus(hWndHost); // 接続失敗
				return 0L;
			}
			EnableWindow(hWndHost, FALSE);		 // [HostName]無効
			EnableWindow(hWndConnect, FALSE);	 // [接続]    無効
			EnableWindow(hWndAccept, FALSE);	 // [接続待ち]無効
			EnableWindow(hWndReject, TRUE);		 // [切断]    有効
			EnableWindow(hWndSendANS, FALSE);	 // [回答欄]  無効   //編集開始
			EnableWindow(hWndAnswer, FALSE);	 // [回答]    無効   //編集終
			EnableWindow(hWndBlack, FALSE);		 // [黒]      無効 (追加1/4)
			EnableWindow(hWndRed, FALSE);			 // [赤]      無効 (追加1/4)
			EnableWindow(hWndGray, FALSE);		 // [消しゴム]無効 (追加1/10)
			EnableWindow(hWndBrown, FALSE);		 // [茶]      無効
			EnableWindow(hWndYellow, FALSE);	 // [黄]      無効
			EnableWindow(hWndBlue, FALSE);		 // [青]      無効
			EnableWindow(hWndGreen, FALSE);		 // [緑]      無効 (ここまで1/10)
			EnableWindow(hWndAllClear, FALSE); // [全消し]      無効 1/23
			EnableWindow(hWndSkip, FALSE);		 // [スキップ]  無効  //// 追加 1/8
			ansflag = 1;											 ////////////////////////////////////回答側（追加）
			return 0L;

		case IDB_SEND: //[切断要請]ボタン押下
			if (send(sock, "REJECT", 7, 0) == SOCKET_ERROR)
			{ // 送信処理
				// 送信に失敗したらエラーを表示
				MessageBox(hWnd, TEXT("sending failed"), TEXT("Error"),
									 MB_OK | MB_ICONEXCLAMATION);
			}
			return 0L;

		case IDB_REJECT: // [切断]ボタン押下
			if (sock != INVALID_SOCKET)
			{ // 自分がクライアント側なら
				// ソケットを閉じる
				closesocket(sock);
				sock = INVALID_SOCKET;
			}
			if (sv_sock != INVALID_SOCKET)
			{ // 自分がサーバ側なら
				// サーバ用ソケットを閉じる
				closesocket(sv_sock);
				sv_sock = INVALID_SOCKET;
			}
			phe = NULL;
			DestroyWindow(hWnd);
			return 0L;

		case IDB_ANSWER:																		// [回答]ボタン押下  //編集開始
			char buf[MAX_MESSAGE];														// 送信内容を一時的に格納するバッファ
			GetWindowText(hWndSendANS, buf, sizeof(buf) - 1); // 回答入力欄の内容を取得
			SetWindowText(hWndAnsMSG, buf);										// 追加 1/8
			if (send(sock, buf, strlen(buf) + 1, 0) == SOCKET_ERROR)
			{ // 送信処理
				// 送信に失敗したらエラーを表示
				MessageBox(hWnd, TEXT("sending failed"), TEXT("Error"),
									 MB_OK | MB_ICONEXCLAMATION);
			}
			SetWindowText(hWndSendANS, TEXT("")); // 回答入力用エディットボックスを空にする
			SetFocus(hWndSendANS);								// フォーカス指定
			return 0L;														// 編集終

		//////////////////追加 1/8 ///////////////////////////////////////////////////////////
		case IDB_SKIP: // [スキップ]ボタン押下
			if (send(sock, "SKIP", 5, 0) == SOCKET_ERROR)
			{ // 送信処理(お題表示の切り替え）
				// 送信に失敗したらエラーを表示
				MessageBox(hWnd, TEXT("sending failed"), TEXT("Error"),
									 MB_OK | MB_ICONEXCLAMATION);
			}
			resetData();																					/////自分の描写情報を空に
			InvalidateRect(hWnd, &d, TRUE);												////再描画命令
			EnableWindow(hWndSendANS, TRUE);											// [回答欄]    有効   //編集開始
			EnableWindow(hWndAnswer, TRUE);												// [回答]      有効   //編集終
			EnableWindow(hWndSkip, FALSE);												// [スキップ]  無効
			EnableWindow(hWndBlack, FALSE);												// [黒]      無効 (追加1/4)
			EnableWindow(hWndRed, FALSE);													// [赤]      無効 (追加1/4)
			EnableWindow(hWndGray, FALSE);												// [消しゴム]無効 (追加1/10)
			EnableWindow(hWndBrown, FALSE);												// [茶]      無効
			EnableWindow(hWndYellow, FALSE);											// [黄]      無効
			EnableWindow(hWndBlue, FALSE);												// [青]      無効
			EnableWindow(hWndGreen, FALSE);												// [緑]      無効 (ここまで1/10)
			EnableWindow(hWndAllClear, FALSE);										// [全消し]      無効 1/23
			SetWindowText(hWndSubMSG, "");												// お題の削除
			SetWindowText(hWndPositionMSG, "あなたは回答側です"); //// 追加 1/10
			ansflag = 1;
			return 0L;
			//////////////////ここまで//////////////////////////////////////////////////////////
		case IDB_BLACK: //[黒]ボタン押下 (追加 1/4)
			if (ansflag == 2)
			{ // お絵描き側
				if (send(sock, "BLACK", 6, 0) == SOCKET_ERROR)
				{ // 送信処理
					// 送信に失敗したらエラーを表示
					MessageBox(hWnd, TEXT("sending failed"), TEXT("Error"),
										 MB_OK | MB_ICONEXCLAMATION);
				}
				else
				{								 // 送信できたなら
					color = BLACK; // 色を黒に変える
				}
			}
			return 0L;	// (ここまで 1/4)
		case IDB_RED: //[赤]ボタン押下 (追加 1/4)
			if (ansflag == 2)
			{ // お絵描き側
				if (send(sock, "RED", 4, 0) == SOCKET_ERROR)
				{ // 送信処理
					// 送信に失敗したらエラーを表示
					MessageBox(hWnd, TEXT("sending failed"), TEXT("Error"),
										 MB_OK | MB_ICONEXCLAMATION);
				}
				else
				{							 // 送信できたなら
					color = RED; // 色を赤に変える
				}
			}
			return 0L;	 // (ここまで 1/4)
		case IDB_GRAY: //[消しゴム]ボタン押下 (追加 1/10)
			if (ansflag == 2)
			{ // お絵描き側
				if (send(sock, "GRAY", 5, 0) == SOCKET_ERROR)
				{ // 送信処理
					// 送信に失敗したらエラーを表示
					MessageBox(hWnd, TEXT("sending failed"), TEXT("Error"),
										 MB_OK | MB_ICONEXCLAMATION);
				}
				else
				{								// 送信できたなら
					color = GRAY; // 色を灰色に変える
				}
			}
			return 0L;		// (ここまで 1/10)
		case IDB_BROWN: //[茶]ボタン押下 (追加 1/10)
			if (ansflag == 2)
			{ // お絵描き側
				if (send(sock, "BROWN", 6, 0) == SOCKET_ERROR)
				{ // 送信処理
					// 送信に失敗したらエラーを表示
					MessageBox(hWnd, TEXT("sending failed"), TEXT("Error"),
										 MB_OK | MB_ICONEXCLAMATION);
				}
				else
				{								 // 送信できたなら
					color = BROWN; // 色を茶色に変える
				}
			}
			return 0L;		 // (ここまで 1/10)
		case IDB_YELLOW: //[黄]ボタン押下 (追加 1/10)
			if (ansflag == 2)
			{ // お絵描き側
				if (send(sock, "YELLOW", 7, 0) == SOCKET_ERROR)
				{ // 送信処理
					// 送信に失敗したらエラーを表示
					MessageBox(hWnd, TEXT("sending failed"), TEXT("Error"),
										 MB_OK | MB_ICONEXCLAMATION);
				}
				else
				{									// 送信できたなら
					color = YELLOW; // 色を黄に変える
				}
			}
			return 0L;	 // (ここまで 1/10)
		case IDB_BLUE: //[青]ボタン押下 (追加 1/10)
			if (ansflag == 2)
			{ // お絵描き側
				if (send(sock, "BLUE", 5, 0) == SOCKET_ERROR)
				{ // 送信処理
					// 送信に失敗したらエラーを表示
					MessageBox(hWnd, TEXT("sending failed"), TEXT("Error"),
										 MB_OK | MB_ICONEXCLAMATION);
				}
				else
				{								// 送信できたなら
					color = BLUE; // 色を青に変える
				}
			}
			return 0L;		// (ここまで 1/10)
		case IDB_GREEN: //[緑]ボタン押下 (追加 1/10)
			if (ansflag == 2)
			{ // お絵描き側
				if (send(sock, "GREEN", 6, 0) == SOCKET_ERROR)
				{ // 送信処理
					// 送信に失敗したらエラーを表示
					MessageBox(hWnd, TEXT("sending failed"), TEXT("Error"),
										 MB_OK | MB_ICONEXCLAMATION);
				}
				else
				{								 // 送信できたなら
					color = GREEN; // 色を緑に変える
				}
			}
			return 0L; // (ここまで 1/10

		case IDB_ALLCLEAR:								//[全消し]ボタン押下 (追加 1/23)
			resetData();										/////自分の描写情報を空に
			InvalidateRect(hWnd, &d, TRUE); ////再描画命令
			if (send(sock, "CLEAR", 6, 0) == SOCKET_ERROR)
			{ // 送信処理
				// 送信に失敗したらエラーを表示
				MessageBox(hWnd, TEXT("sending failed"), TEXT("Error"),
									 MB_OK | MB_ICONEXCLAMATION);
			}
			return 0L; // (ここまで 1/23
		}
		/* end of switch (LOWORD(wP)) */
		return 0L;

	case WM_SOCKET: // 非同期処理メッセージ
		if (WSAGETSELECTERROR(lP) != 0)
		{
			return 0L;
		}

		switch (WSAGETSELECTEVENT(lP))
		{
		case FD_ACCEPT: // 接続待ち完了通知
		{
			SOCKADDR_IN cl_sin;
			int len = sizeof(cl_sin);
			sock = accept(sv_sock, (LPSOCKADDR)&cl_sin, &len);

			if (sock == INVALID_SOCKET)
			{
				MessageBox(hWnd, "Accepting connection failed",
									 "Error", MB_OK | MB_ICONEXCLAMATION);
				closesocket(sv_sock);
				sv_sock = INVALID_SOCKET;
				EnableWindow(hWndHost, TRUE);			 // [HostName]有効
				EnableWindow(hWndConnect, TRUE);	 // [接続]    有効
				EnableWindow(hWndAccept, TRUE);		 // [接続待ち]有効
				EnableWindow(hWndReject, FALSE);	 // [切断]    無効
				EnableWindow(hWndSend, FALSE);		 // [切断要請]  無効
				EnableWindow(hWndSendANS, FALSE);	 // [回答欄]    無効   //編集開始
				EnableWindow(hWndAnswer, FALSE);	 // [回答]      無効   //編集終
				EnableWindow(hWndBlack, FALSE);		 // [黒]        無効 (追加1/4)
				EnableWindow(hWndRed, FALSE);			 // [赤]        無効 (追加1/4)
				EnableWindow(hWndGray, FALSE);		 // [消しゴム]  無効 (追加1/10)
				EnableWindow(hWndBrown, FALSE);		 // [茶]        無効
				EnableWindow(hWndYellow, FALSE);	 // [黄]        無効
				EnableWindow(hWndBlue, FALSE);		 // [青]        無効
				EnableWindow(hWndGreen, FALSE);		 // [緑]        無効 (ここまで1/10)
				EnableWindow(hWndAllClear, FALSE); // [全消し]      無効 1/23
				EnableWindow(hWndSkip, FALSE);		 // [スキップ]  無効  //// 追加 1/8
				SetFocus(hWndHost);								 // フォーカス指定
				return 0L;
			}

#ifndef NO_DNS
			// ホスト名取得
			phe = gethostbyaddr((char *)&cl_sin.sin_addr, 4, AF_INET);
			if (phe)
			{
				SetWindowText(hWndHost, phe->h_name);
			}
#endif // NO_DNS

			// 非同期モード (受信＆切断）
			if (WSAAsyncSelect(sock, hWnd, WM_SOCKET, FD_READ | FD_CLOSE) == SOCKET_ERROR)
			{
				// 接続に失敗したら初期状態に戻す
				MessageBox(hWnd, "WSAAsyncSelect() failed",
									 "Error", MB_OK | MB_ICONEXCLAMATION);
				EnableWindow(hWndHost, TRUE);			 // [HostName]有効
				EnableWindow(hWndConnect, TRUE);	 // [接続]    有効
				EnableWindow(hWndAccept, TRUE);		 // [接続待ち]有効
				EnableWindow(hWndReject, FALSE);	 // [切断]    無効
				EnableWindow(hWndSend, FALSE);		 // [切断要請]  無効
				EnableWindow(hWndSendANS, FALSE);	 // [回答欄]    無効   //編集開始
				EnableWindow(hWndAnswer, FALSE);	 // [回答]      無効   //編集終
				EnableWindow(hWndBlack, FALSE);		 // [黒]        無効 (追加1/4)
				EnableWindow(hWndRed, FALSE);			 // [赤]        無効 (追加1/4)
				EnableWindow(hWndGray, FALSE);		 // [消しゴム]  無効 (追加1/10)
				EnableWindow(hWndBrown, FALSE);		 // [茶]        無効
				EnableWindow(hWndYellow, FALSE);	 // [黄]        無効
				EnableWindow(hWndBlue, FALSE);		 // [青]        無効
				EnableWindow(hWndGreen, FALSE);		 // [緑]        無効 (ここまで1/10)
				EnableWindow(hWndAllClear, FALSE); // [全消し]      無効 1/23
				EnableWindow(hWndSkip, FALSE);		 // [スキップ]  無効  //// 追加 1/8
				SetFocus(hWndHost);								 // フォーカス指定
				return 0L;
			}
			EnableWindow(hWndSend, TRUE);			// [切断要請]    有効
			EnableWindow(hWndSendANS, FALSE); // [回答欄]      無効   //編集開始
			EnableWindow(hWndAnswer, FALSE);	// [回答]        無効   //編集終
			EnableWindow(hWndBlack, TRUE);		// [黒]      有効 (追加1/4)
			EnableWindow(hWndRed, TRUE);			// [赤]      有効 (追加1/4)
			EnableWindow(hWndGray, TRUE);			// [消しゴム]有効 (追加1/10)
			EnableWindow(hWndBrown, TRUE);		// [茶]      有効
			EnableWindow(hWndYellow, TRUE);		// [黄]      有効
			EnableWindow(hWndBlue, TRUE);			// [青]      有効
			EnableWindow(hWndGreen, TRUE);		// [緑]      有効 (ここまで1/10)
			EnableWindow(hWndAllClear, TRUE); // [全消し]      有効 1/23
			EnableWindow(hWndSkip, TRUE);			// [スキップ]  有効  //// 追加 1/8
			active = 1;
			/////////////* 追加部分 */////////////////////
			int randval;
			srand((unsigned int)time(NULL));
			randval = rand() % 3;
			if (randval == 0)
			{
				strcpy_s(subject, 20, "いぬ");
			}
			else if (randval == 1)
			{
				strcpy_s(subject, 20, "ねこ");
			}
			else if (randval == 2)
			{
				strcpy_s(subject, 20, "うさぎ");
			}
			if (send(sock, subject, 7, 0) == SOCKET_ERROR)
			{ // 送信処理
				// 送信に失敗したらエラーを表示
				MessageBox(hWnd, TEXT("sending failed"), TEXT("Error"),
									 MB_OK | MB_ICONEXCLAMATION);
			}
			SetWindowText(hWndSubMSG, subject);												// お題表示
																																//////////////////////////////////////////////////////////////////////////]
			SetWindowText(hWndPositionMSG, "あなたはお絵描き側です"); //// 追加 1/10
			return 0L;
		} /* end of case FD_ACCEPT: */

		case FD_CONNECT: // 接続完了通知
			// 非同期モード (受信＆切断)
			if (WSAAsyncSelect(sock, hWnd, WM_SOCKET, FD_READ | FD_CLOSE) == SOCKET_ERROR)
			{
				// 接続に失敗したら初期状態に戻す
				MessageBox(hWnd, "WSAAsyncSelect() failed",
									 "Error", MB_OK | MB_ICONEXCLAMATION);
				EnableWindow(hWndHost, TRUE);			 // [HostName]有効
				EnableWindow(hWndConnect, TRUE);	 // [接続]    有効
				EnableWindow(hWndAccept, TRUE);		 // [接続待ち]有効
				EnableWindow(hWndReject, FALSE);	 // [切断]    無効
				EnableWindow(hWndSend, FALSE);		 // [切断要請]  無効
				EnableWindow(hWndSendANS, FALSE);	 // [回答欄]    無効   //編集開始
				EnableWindow(hWndAnswer, FALSE);	 // [回答]      無効   //編集終
				EnableWindow(hWndBlack, FALSE);		 // [黒]        無効 (追加1/4)
				EnableWindow(hWndRed, FALSE);			 // [赤]        無効 (追加1/4)
				EnableWindow(hWndGray, FALSE);		 // [消しゴム]  無効 (追加1/10)
				EnableWindow(hWndBrown, FALSE);		 // [茶]        無効
				EnableWindow(hWndYellow, FALSE);	 // [黄]        無効
				EnableWindow(hWndBlue, FALSE);		 // [青]        無効
				EnableWindow(hWndGreen, FALSE);		 // [緑]        無効 (ここまで1/10)
				EnableWindow(hWndAllClear, FALSE); // [全消し]      無効 1/23
				EnableWindow(hWndSkip, FALSE);		 // [スキップ]  無効  //// 追加 1/8
				SetFocus(hWndHost);								 // フォーカス指定
				return 0L;
			}
			EnableWindow(hWndSend, TRUE);													// [切断要請]    有効
			EnableWindow(hWndSendANS, TRUE);											// [回答欄]      有効   //編集開始
			EnableWindow(hWndAnswer, TRUE);												// [回答]        有効   //編集終
			EnableWindow(hWndBlack, FALSE);												// [黒]          無効 (追加1/4)
			EnableWindow(hWndRed, FALSE);													// [赤]          無効 (追加1/4)
			EnableWindow(hWndGray, FALSE);												// [消しゴム]  無効 (追加1/10)
			EnableWindow(hWndBrown, FALSE);												// [茶]        無効
			EnableWindow(hWndYellow, FALSE);											// [黄]        無効
			EnableWindow(hWndBlue, FALSE);												// [青]        無効
			EnableWindow(hWndGreen, FALSE);												// [緑]        無効 (ここまで1/10)
			EnableWindow(hWndAllClear, FALSE);										// [全消し]      無効 1/23
			EnableWindow(hWndSkip, FALSE);												// [スキップ]  無効  //// 追加 1/8
			SetWindowText(hWndPositionMSG, "あなたは回答側です"); //// 追加 1/10
			active = 1;
			return 0L;

		case FD_READ:							// メッセージ受信(追加)
			char buf2[MAX_MESSAGE]; // 受信内容を一時的に格納するバッファ
			int flag2, x2, y2;
			if (ansflag == 1)
			{ // 回答者側(追加)
				if (recv(sock, buf2, 8, 0) != SOCKET_ERROR)
				{ // 受信できたなら
					if (strcmp(buf2, "REJECT") == 0)
					{
						MessageBox(hWnd, TEXT("相手から切断要請が送られてきました。切断ボタンを押し、通信を切断してください"), TEXT("Information"),
											 MB_OK | MB_ICONINFORMATION);
					}
					////// 回答側でお題をランダム表示する(追加)12/20
					else if (strcmp(buf2, "SWITCH") == 0)
					{
						srand((unsigned int)time(NULL));
						randval = rand() % 3;
						if (randval == 0)
						{
							strcpy_s(subject, 20, "いぬ");
						}
						else if (randval == 1)
						{
							strcpy_s(subject, 20, "ねこ");
						}
						else if (randval == 2)
						{
							strcpy_s(subject, 20, "うさぎ");
						}
						resetData2();												//////相手の描写情報を空に,12/20追加
						InvalidateRect(hWnd, &d, TRUE);			////再描画命令,12/20追加
						SetWindowText(hWndSubMSG, subject); // お題表示
						SetWindowText(hWndRightMSG, "正解です");
						EnableWindow(hWndSendANS, FALSE);													// [回答欄]    無効   //編集開始
						EnableWindow(hWndAnswer, FALSE);													// [回答]      無効   //編集終
						EnableWindow(hWndBlack, TRUE);														// [黒]        有効 (追加1/4)
						EnableWindow(hWndRed, TRUE);															// [赤]        有効 (追加1/4)
						EnableWindow(hWndGray, TRUE);															// [消しゴム]有効 (追加1/10)
						EnableWindow(hWndBrown, TRUE);														// [茶]      有効
						EnableWindow(hWndYellow, TRUE);														// [黄]      有効
						EnableWindow(hWndBlue, TRUE);															// [青]      有効
						EnableWindow(hWndGreen, TRUE);														// [緑]      有効 (ここまで1/10)
						EnableWindow(hWndAllClear, TRUE);													// [全消し]      有効 1/23
						EnableWindow(hWndSkip, TRUE);															// [スキップ]  有効  //// 追加 1/8
						SetWindowText(hWndPositionMSG, "あなたはお絵描き側です"); //// 追加 1/10
						ansflag = 2;
					}
					else if (strcmp(buf2, "FALSE") == 0)
					{
						SetWindowText(hWndRightMSG, "不正解です");
					}
					////////////////////////追加 1/8 /////////////////////////
					else if (strcmp(buf2, "SKIP") == 0)
					{
						srand((unsigned int)time(NULL));
						randval = rand() % 3;
						if (randval == 0)
						{
							strcpy_s(subject, 20, "いぬ");
						}
						else if (randval == 1)
						{
							strcpy_s(subject, 20, "ねこ");
						}
						else if (randval == 2)
						{
							strcpy_s(subject, 20, "うさぎ");
						}
						resetData2();										//////相手の描写情報を空に
						InvalidateRect(hWnd, &d, TRUE); ////再描画命令
						MessageBox(hWnd, TEXT("相手がスキップボタンを押しました。あなたがお絵描き側です"), TEXT("Information"),
											 MB_OK | MB_ICONINFORMATION);
						SetWindowText(hWndPositionMSG, "あなたはお絵描き側です"); //// 追加 1/10
						SetWindowText(hWndSubMSG, subject);												// お題表示
						EnableWindow(hWndSendANS, FALSE);													// [回答欄]      無効   //編集開始(12/20)
						EnableWindow(hWndAnswer, FALSE);													// [回答]        無効   //編集終(12/20)
						EnableWindow(hWndSkip, TRUE);															// [スキップ]  有効  //// 追加 1/8
						EnableWindow(hWndBlack, TRUE);														// [黒]      有効 (追加1/4)
						EnableWindow(hWndRed, TRUE);															// [赤]      有効 (追加1/4)
						EnableWindow(hWndGray, TRUE);															// [消しゴム]有効 (追加1/10)
						EnableWindow(hWndBrown, TRUE);														// [茶]      有効
						EnableWindow(hWndYellow, TRUE);														// [黄]      有効
						EnableWindow(hWndBlue, TRUE);															// [青]      有効
						EnableWindow(hWndGreen, TRUE);														// [緑]      有効 (ここまで1/10)
						EnableWindow(hWndAllClear, TRUE);													// [全消し]      有効 1/23
						ansflag = 2;
					}
					/////////////////////////ここまで/////////////////
					else if (strcmp(buf2, "BLACK") == 0)
					{ // (追加 1/4)
						color2 = BLACK;
					} // (ここまで 1/4)
					else if (strcmp(buf2, "RED") == 0)
					{ // (追加 1/4)
						color2 = RED;
					} // (ここまで 1/4)
					else if (strcmp(buf2, "GRAY") == 0)
					{ // (追加 1/10)
						color2 = GRAY;
					} // (ここまで 1/10)
					else if (strcmp(buf2, "BROWN") == 0)
					{ // (追加 1/10)
						color2 = BROWN;
					} // (ここまで 1/10)
					else if (strcmp(buf2, "YELLOW") == 0)
					{ // (追加 1/10)
						color2 = YELLOW;
					} // (ここまで 1/10)
					else if (strcmp(buf2, "BLUE") == 0)
					{ // (追加 1/10)
						color2 = BLUE;
					} // (ここまで 1/10)
					else if (strcmp(buf2, "GREEN") == 0)
					{ // (追加 1/10)
						color2 = GREEN;
					} // (ここまで 1/10)
					else if (strcmp(buf2, "CLEAR") == 0)
					{																	// 1/23
						resetData2();										//////相手の描写情報を空に
						InvalidateRect(hWnd, &d, TRUE); ////再描画命令
					}
					else
					{ // (編集 1/4)
						sscanf_s(buf2, "%1d%3d%3d", &flag2, &x2, &y2);
						setData2(flag2, x2, y2);
						InvalidateRect(hWnd, &d, FALSE);
						n2++;
					} // (編集 1/4)
				}
			}

			if (ansflag == 2)
			{ // お題を出す側(追加)
				if (recv(sock, buf2, 20, 0) != SOCKET_ERROR)
				{ // 受信できたなら  ////受信するのを8から20に変更追加1/23
					if (strcmp(buf2, "REJECT") == 0)
					{
						MessageBox(hWnd, TEXT("相手から切断要請が送られてきました。切断ボタンを押し、通信を切断してください"), TEXT("Information"),
											 MB_OK | MB_ICONINFORMATION);
					}
					else
					{
						if (strcmp(buf2, subject) == 0)
						{
							SetWindowText(hWndAnsMSG, buf2); // 受信メッセージ表示用エディットボックスに受信内容を貼りつける
							SetWindowText(hWndRightMSG, "正解です");
							resetData();										/////自分の描写情報を空に,12/20追加
							InvalidateRect(hWnd, &d, TRUE); ////再描画命令,12/20追加
							// 追加部分12/20/////////////////////////////////////
							SetWindowText(hWndSubMSG, ""); // お題の削除
							if (send(sock, "SWITCH", 7, 0) == SOCKET_ERROR)
							{ // 送信処理(お題表示の切り替え）
								// 送信に失敗したらエラーを表示
								MessageBox(hWnd, TEXT("sending failed"), TEXT("Error"),
													 MB_OK | MB_ICONEXCLAMATION);
							}
							// このタイミングで切り替わるので…
							EnableWindow(hWndSendANS, TRUE);											// [回答欄]      有効   //編集開始(12/20)
							EnableWindow(hWndAnswer, TRUE);												// [回答]        有効   //編集終(12/20)
							EnableWindow(hWndBlack, FALSE);												// [黒]          無効 (追加1/4)
							EnableWindow(hWndRed, FALSE);													// [赤]          無効 (追加1/4)
							EnableWindow(hWndGray, FALSE);												// [消しゴム]  無効 (追加1/10)
							EnableWindow(hWndBrown, FALSE);												// [茶]        無効
							EnableWindow(hWndYellow, FALSE);											// [黄]        無効
							EnableWindow(hWndBlue, FALSE);												// [青]        無効
							EnableWindow(hWndGreen, FALSE);												// [緑]        無効 (ここまで1/10)
							EnableWindow(hWndAllClear, FALSE);										// [全消し]      無効 1/23
							EnableWindow(hWndSkip, FALSE);												// [スキップ]  無効  //// 追加 1/8
							SetWindowText(hWndPositionMSG, "あなたは回答側です"); //// 追加 1/10
							ansflag = 1;
							/////////////////////ここまで///////////////////////
						}
						else
						{
							SetWindowText(hWndAnsMSG, buf2); // 受信メッセージ表示用エディットボックスに受信内容を貼りつける
							SetWindowText(hWndRightMSG, "不正解です");
							if (send(sock, "FALSE", 6, 0) == SOCKET_ERROR)
							{ // 送信処理(不正解したことを送信）
								// 送信に失敗したらエラーを表示
								MessageBox(hWnd, TEXT("sending failed"), TEXT("Error"),
													 MB_OK | MB_ICONEXCLAMATION);
							}
						}
					}
				}
			}
			return 0L;

		case FD_CLOSE: // 切断された
			MessageBox(hWnd, "切断されました。",
								 "Information", MB_OK | MB_ICONINFORMATION);
			SendMessage(hWnd, WM_COMMAND, IDB_REJECT, 0); // 切断処理発行

			return 0L;
		} /* end of switch (WSAGETSELECTEVENT(lP)) */
		return 0L;

	case WM_LBUTTONDOWN:		 // マウス左ボタンが押された
		char buf[MAX_MESSAGE]; // 座標を格納するバッファ
		if (ansflag == 2)
		{ // お絵かき側だけがキャンバスに書ける(追加)  //(編集 1/4)
			if (checkMousePos(LOWORD(lP), HIWORD(lP)) && active == 1)
			{																			// 描画領域の中なら
				setData(0, LOWORD(lP), HIWORD(lP)); // 線の始点として座標を記録

				sprintf_s(buf, "%1d%03d%03d", 0, LOWORD(lP), HIWORD(lP));
				if (send(sock, buf, 8, 0) == SOCKET_ERROR)
				{ // 送信処理
					// 送信に失敗したらエラーを表示
					MessageBox(hWnd, TEXT("sending failed"), TEXT("Error"),
										 MB_OK | MB_ICONEXCLAMATION);
				}
				InvalidateRect(hWnd, &d, FALSE);
				n++;
				mouseFlg = TRUE;
			}
			else
			{
				mouseFlg = FALSE;
			}
			// (ここまで 1/4)
		}
		return 0L;

	case WM_MOUSEMOVE: // マウスポインタが移動した
		if (ansflag == 2)
		{ // お絵かき側だけがキャンバスにかけるように(追加)
			if (wP == MK_LBUTTON)
			{ // 左ボタンが押されている
				if (checkMousePos(LOWORD(lP), HIWORD(lP)) && active == 1)
				{ // 描画領域の中なら
					// (編集 1/4)
					if (mouseFlg)
					{																			// 前回描画しているなら
						setData(1, LOWORD(lP), HIWORD(lP)); // 線の途中として座標を記録
						sprintf_s(buf, "%1d%03d%03d", 1, LOWORD(lP), HIWORD(lP));
						if (send(sock, buf, 8, 0) == SOCKET_ERROR)
						{ // 送信処理
							// 送信に失敗したらエラーを表示
							MessageBox(hWnd, TEXT("sending failed"), TEXT("Error"),
												 MB_OK | MB_ICONEXCLAMATION);
						}
					}
					else
					{																			// 前回描画していないなら
						setData(0, LOWORD(lP), HIWORD(lP)); // 線の始点として座標を記録
						sprintf_s(buf, "%1d%03d%03d", 0, LOWORD(lP), HIWORD(lP));
						if (send(sock, buf, 8, 0) == SOCKET_ERROR)
						{ // 送信処理
							// 送信に失敗したらエラーを表示
							MessageBox(hWnd, TEXT("sending failed"), TEXT("Error"),
												 MB_OK | MB_ICONEXCLAMATION);
						}
					}

					mouseFlg = TRUE;
					InvalidateRect(hWnd, &d, FALSE);
					n++;
				}
				else
				{ // 描画領域の外なら
					mouseFlg = FALSE;
				} // (ここまで 1/4)
			}
		}
		return 0L;

	case WM_PAINT: // 再描画
		return OnPaint(hWnd, uMsg, wP, lP);

		return 0L;

	case WM_SETFOCUS: // ウィンドウにフォーカスが来たら
		// ホスト名入力欄が入力可ならフォーカス
		if (IsWindowEnabled(hWndHost))
		{
			SetFocus(hWndHost);
		}
		return 0L;

	case WM_DESTROY: // ウィンドウが破棄された
		closesocket(sock);
		DeleteObject(hPenBlack);	// 黒ペン削除
		DeleteObject(hPenRed);		// 赤ペン削除
		DeleteObject(hPenGray);		// 灰色ペン削除(追加1/10)
		DeleteObject(hPenBrown);	// 茶ペン削除
		DeleteObject(hPenYellow); // 黄ペン削除
		DeleteObject(hPenBlue);		// 青ペン削除
		DeleteObject(hPenGreen);	// 緑ペン削除(ここまで1/10)
		PostQuitMessage(0);
		return 0L;

	default:
		return DefWindowProc(hWnd, uMsg, wP, lP); // 標準メッセージ処理
	} /* end of switch (uMsg) */
}

////////////////////////////////////////////////////////////////////////////////
//
//  ソケット初期化処理
//
BOOL SockInit(HWND hWnd)
{
	WSADATA wsa;
	int ret;
	char ret_buf[80];

	ret = WSAStartup(MAKEWORD(1, 1), &wsa);

	if (ret != 0)
	{
		wsprintf(ret_buf, "%d is the err", ret);
		MessageBox(hWnd, ret_buf, "Error", MB_OK | MB_ICONSTOP);
		exit(-1);
	}
	return FALSE;
}

////////////////////////////////////////////////////////////////////////////////
//
//  ソケット接続 (クライアント側)
//
BOOL SockConnect(HWND hWnd, LPCSTR host)
{
	SOCKADDR_IN cl_sin; // SOCKADDR_IN構造体

	// ソケットを開く
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET)
	{ // ソケット作成失敗
		MessageBox(hWnd, "Socket() failed", "Error", MB_OK | MB_ICONEXCLAMATION);
		return TRUE;
	}

	memset(&cl_sin, 0x00, sizeof(cl_sin)); // 構造体初期化
	cl_sin.sin_family = AF_INET;					 // インターネット
	cl_sin.sin_port = htons(PORT);				 // ポート番号指定

	phe = gethostbyname(host); // アドレス取得

	if (phe == NULL)
	{
		MessageBox(hWnd, "gethostbyname() failed.",
							 "Error", MB_OK | MB_ICONEXCLAMATION);
		return TRUE;
	}
	memcpy(&cl_sin.sin_addr, phe->h_addr, phe->h_length);

	// 非同期モード (接続)
	if (WSAAsyncSelect(sock, hWnd, WM_SOCKET, FD_CONNECT) == SOCKET_ERROR)
	{
		closesocket(sock);
		sock = INVALID_SOCKET;
		MessageBox(hWnd, "WSAAsyncSelect() failed",
							 "Error", MB_OK | MB_ICONEXCLAMATION);
		return TRUE;
	}

	// 接続処理
	if (connect(sock, (LPSOCKADDR)&cl_sin, sizeof(cl_sin)) == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSAEWOULDBLOCK)
		{
			closesocket(sock);
			sock = INVALID_SOCKET;
			MessageBox(hWnd, "connect() failed", "Error", MB_OK | MB_ICONEXCLAMATION);
			return TRUE;
		}
	}
	return FALSE;
}

////////////////////////////////////////////////////////////////////////////////
//
//  接続待ち (サーバ側)
//
BOOL SockAccept(HWND hWnd)
{
	SOCKADDR_IN sv_sin; // SOCKADDR_IN構造体

	// サーバ用ソケット
	sv_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sv_sock == INVALID_SOCKET)
	{ // ソケット作成失敗
		MessageBox(hWnd, "Socket() failed", "Error", MB_OK | MB_ICONEXCLAMATION);
		return TRUE;
	}

	memset(&sv_sin, 0x00, sizeof(sv_sin));			// 構造体初期化
	sv_sin.sin_family = AF_INET;								// インターネット
	sv_sin.sin_port = htons(PORT);							// ポート番号指定
	sv_sin.sin_addr.s_addr = htonl(INADDR_ANY); // アドレス指定

	if (bind(sv_sock, (LPSOCKADDR)&sv_sin, sizeof(sv_sin)) == SOCKET_ERROR)
	{
		closesocket(sv_sock);
		sv_sock = INVALID_SOCKET;
		MessageBox(hWnd, "bind() failed", "Error", MB_OK | MB_ICONEXCLAMATION);
		return TRUE;
	}

	if (listen(sv_sock, 5) == SOCKET_ERROR)
	{
		// 接続待ち失敗
		closesocket(sv_sock);
		sv_sock = INVALID_SOCKET;
		MessageBox(hWnd, "listen() failed", "Error", MB_OK | MB_ICONEXCLAMATION);
		return TRUE;
	}

	// 非同期処理モード (接続待ち)
	if (WSAAsyncSelect(sv_sock, hWnd, WM_SOCKET, FD_ACCEPT) == SOCKET_ERROR)
	{
		closesocket(sv_sock);
		sv_sock = INVALID_SOCKET;
		MessageBox(hWnd, "WSAAsyncSelect() failed",
							 "Error", MB_OK | MB_ICONEXCLAMATION);
		return TRUE;
	}
	return FALSE;
}

////////////////////////////////////////////////////////////////////////////////
//
//  描画関数
//
LRESULT CALLBACK OnPaint(HWND hWnd, UINT uMsg, WPARAM wP, LPARAM lP)
{
	HDC hdc;
	PAINTSTRUCT ps;

	hdc = BeginPaint(hWnd, &ps);

	// 描画領域の初期化
	MoveToEx(hdc, d.left, d.top, NULL);
	LineTo(hdc, d.right, d.top);		// 上横線
	LineTo(hdc, d.right, d.bottom); // 右縦線
	LineTo(hdc, d.left, d.bottom);	// 下横線
	LineTo(hdc, d.left, d.top);			// 左縦線
	// Rectangle(hdc, 10, 90, 810, 690);//(追加)
	SelectObject(hdc, hPenBlack);
	// SelectObject(hdc, hPenBlack);//(追加)

	for (int i = 0; i < n; i++)
	{ // 線を描画
		if (point_color[i] == BLACK)
		{
			SelectObject(hdc, hPenBlack);
		}
		else if (point_color[i] == RED)
		{
			SelectObject(hdc, hPenRed);
		}
		else if (point_color[i] == GRAY)
		{
			SelectObject(hdc, hPenGray);
		}
		else if (point_color[i] == BROWN)
		{
			SelectObject(hdc, hPenBrown);
		}
		else if (point_color[i] == YELLOW)
		{
			SelectObject(hdc, hPenYellow);
		}
		else if (point_color[i] == BLUE)
		{
			SelectObject(hdc, hPenBlue);
		}
		else if (point_color[i] == GREEN)
		{
			SelectObject(hdc, hPenGreen);
		}

		if (flag[i] == 0)
		{ // 開始点なら、始点を移動
			MoveToEx(hdc, pos[i].x, pos[i].y, NULL);
		}
		else
		{ // 途中の点なら線を引く
			LineTo(hdc, pos[i].x, pos[i].y);
		}
	}

	for (int i = 0; i < n2; i++)
	{ // 線を描画
		if (point_color2[i] == BLACK)
		{
			SelectObject(hdc, hPenBlack);
		}
		else if (point_color2[i] == RED)
		{
			SelectObject(hdc, hPenRed);
		}
		else if (point_color2[i] == GRAY)
		{
			SelectObject(hdc, hPenGray);
		}
		else if (point_color2[i] == BROWN)
		{
			SelectObject(hdc, hPenBrown);
		}
		else if (point_color2[i] == YELLOW)
		{
			SelectObject(hdc, hPenYellow);
		}
		else if (point_color2[i] == BLUE)
		{
			SelectObject(hdc, hPenBlue);
		}
		else if (point_color2[i] == GREEN)
		{
			SelectObject(hdc, hPenGreen);
		}

		if (flag2[i] == 0)
		{ // 開始点なら、始点を移動
			MoveToEx(hdc, pos2[i].x, pos2[i].y, NULL);
		}
		else
		{ // 途中の点なら線を引く
			LineTo(hdc, pos2[i].x, pos2[i].y);
		}
	}
	// (ここまで 1/4)

	EndPaint(hWnd, &ps);

	return 0L;
}

////////////////////////////////////////////////////////////////////////////////
//
//  描画情報を格納
//
void setData(int f, int x, int y)
{
	flag[n] = f;
	pos[n].x = x;
	pos[n].y = y;
	point_color[n] = color;
}

////////////////////////////////////////////////////////////////////////////////
//
//  描画情報を格納
//
void setData2(int f, int x, int y)
{
	flag2[n2] = f;
	pos2[n2].x = x;
	pos2[n2].y = y;
	point_color2[n2] = color2;
}

////////////////////////////////////////////////////////////////////////////////
//
//  マウスの位置がキャンパスの中かどうか判定する
//
BOOL checkMousePos(int x, int y)
{
	if (x >= d.left && x <= d.right && y >= d.top && y <= d.bottom)
	{
		return TRUE;
	}
	return FALSE;
}

void resetData()
{ //////追加12/20
	int t;
	int i;
	t = n;
	color = BLACK; // (追加 1/4)
	for (i = 0; i <= t; i++)
	{
		flag[i] = 0;
		pos[i].x = 0;
		pos[i].y = 0;
		point_color[i] = 0;
		// (ここまで 1/4)
	}
	n = 0;
}

void resetData2()
{ /////追加12/20
	int t;
	int i;
	t = n2;
	color2 = BLACK; // (追加 1/4)
	for (i = 0; i <= t; i++)
	{
		flag2[i] = 0;
		pos2[i].x = 0;
		pos2[i].y = 0;
		point_color2[i] = 0;
	}
	n2 = 0;
}