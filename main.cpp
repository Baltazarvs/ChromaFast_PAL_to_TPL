/*
 * ----------------------------------------------------------------------
 * Created 2022 by Nitrocell (Baltazarus)
 *
 * ChromaFast PAL-TPL Converter is fast palette converter that converts
 * PAL's rgb888 format to BGR555 TPL's format (used for SNES). and its
 * 'T','P','L',0x02 header.
 * ----------------------------------------------------------------------
*/

#include "util.h"
#include "resource.h"

static HWND w_EditPAL = nullptr;
static HWND w_ButtonBrowsePAL = nullptr;
static HWND w_ButtonConvert = nullptr;
static HWND w_DlgStaticReview = nullptr;

static std::vector<int> ReviewColorsVector;

void InitUI(HWND);
const char* OpenFilenameDlg(HWND, bool);
std::pair<cf_byte,cf_byte> WordToBytes(cf_word);

LRESULT __stdcall DlgProc_PaletteReview(HWND w_Dlg, UINT Msg, WPARAM wParam, LPARAM lParam);
LRESULT __stdcall SubClassProc_Review(HWND w_Ctl, UINT Msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

LRESULT __stdcall WndProc(HWND w_Handle, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch(Msg)
    {
        case WM_CREATE:
        {
            InitUI(w_Handle);
            break;
        }
        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case ID_BUTTON_PAL_BROWSE:
                {
                    char* buffer = new char[MAX_PATH];
                    strcpy(buffer, OpenFilenameDlg(w_Handle, false));
                    if(strlen(buffer) < 1)
                        break;
                    SetWindowTextA(w_EditPAL, buffer);
                    delete[] buffer;
                    break;
                }
                case ID_BUTTON_CONVERT:
                {
                    if(GetWindowTextLengthA(w_EditPAL) < 1)
                    {
                        MessageBoxA(w_Handle, "Please input .PAL filename.", "Convert", MB_OK);
                        break;
                    }
                    
                    std::vector<cf_byte> ConvertedSNESColors_Split;

                    ConvertedSNESColors_Split.push_back(0x54);
                    ConvertedSNESColors_Split.push_back(0x50);
                    ConvertedSNESColors_Split.push_back(0x4C);
                    ConvertedSNESColors_Split.push_back(0x02);

                    char* buffer = new char[MAX_PATH + 1];
                    GetWindowTextA(w_EditPAL, buffer, MAX_PATH);

                    std::ifstream file(buffer, std::ios::binary);
                    if(file.is_open())
                    {
                        cf_byte colors[256 * 3];
                        file.read(reinterpret_cast<char*>(colors), 256 * 3 * sizeof(cf_byte));
                        for(int i = 0; i < sizeof(colors)/sizeof(cf_byte); ++i)
                        {
                            if(((i + 1) % 3) == 0)
                            {
                                cf_word bgr555_color_result = 0x0000;

                                cf_byte R = colors[i - 2];
                                cf_byte G = colors[i - 1];
                                cf_byte B = colors[i];

                                int color_rgb888 = 0x00000000;
                                color_rgb888 |= ((int)R << 16);
                                color_rgb888 |= ((int)G << 8);
                                color_rgb888 |= (int)B;

                                std::ostringstream ssss;
                                ssss << std::hex << color_rgb888;
                                std::string resultt_col = ssss.str();

                                // Conversion to BGR555 format:
                                int color_bgr555 = 0x00000000;
                                byte leftmost = (byte)(color_rgb888 >> 16);
                                byte middle = (byte)(color_rgb888 >> 8);
                                byte righmost = (byte)(color_rgb888 >> 0);
                                
                                leftmost &= ((leftmost >> 3) << 3);
                                middle &= ((middle >> 3) << 3);
                                righmost &= ((righmost >> 3) << 3);
                                
                                bgr555_color_result |= ((cf_word)B << 7);
                                bgr555_color_result |= ((cf_word)G << 2);
                                bgr555_color_result |= ((cf_word)R >> 3);

                                std::ostringstream sss;
                                sss << std::hex << bgr555_color_result;
                                std::string result_col = sss.str();

                                std::pair<cf_byte,cf_byte> bgr555_color_split = WordToBytes(bgr555_color_result);
                                if(bgr555_color_split.second == 0x00)
                                    ConvertedSNESColors_Split.push_back(0x00);
                                else
                                    ConvertedSNESColors_Split.push_back(bgr555_color_split.second);
                                if(bgr555_color_split.first == 0x00)
                                    ConvertedSNESColors_Split.push_back(0x00);
                                else
                                    ConvertedSNESColors_Split.push_back(bgr555_color_split.first);
                                ReviewColorsVector.push_back(RGB(R, G, B));
                            }
                        }   
                        file.close();
                    }
                    else
                    {
                        MessageBoxA(w_Handle, "Cannot open PAL file!", "Convert", MB_OK);
                        delete[] buffer;
                        break;
                    }

                    if(ConvertedSNESColors_Split.empty() || ConvertedSNESColors_Split.size() < (256ull * 2 + 4))
                    {
                        MessageBoxA(w_Handle, "Invalid Colors!", "Insufficient Colors", MB_OK | MB_ICONEXCLAMATION);
                        delete[] buffer;
                        break;
                    }
                    
                    char* save_path_buffer = new char[MAX_PATH];
                    strcpy(save_path_buffer, OpenFilenameDlg(w_Handle, true));

                    std::ofstream filew(save_path_buffer, std::ios::binary);
                    if(filew.is_open())
                    {
                        cf_byte colors[ConvertedSNESColors_Split.size()];
                        for(int i = 0; i < sizeof(colors)/sizeof(cf_byte); ++i)
                            colors[i] = ConvertedSNESColors_Split[i];
                        filew.write(
                            reinterpret_cast<char*>(colors),
                            ConvertedSNESColors_Split.size()
                        );
                        delete[] save_path_buffer;
                        filew.close();
                    }
                    else
                    {
                        MessageBoxA(w_Handle, "Cannot open TPL File!", "Convert", MB_OK);
                        delete[] buffer;
                        break;
                    }

                    MessageBoxA(w_Handle, "Conversion Successful!", "Convert", MB_OK | MB_ICONINFORMATION);

                    DialogBoxA(
                        GetModuleHandleA(nullptr),
                        MAKEINTRESOURCEA(IDD_REVIEW),
                        w_Handle,
                        reinterpret_cast<DLGPROC>(&DlgProc_PaletteReview)
                    );

                    delete[] buffer;
                    break;
                }
                case ID_FILE_EXIT:
                    DestroyWindow(w_Handle);
                    break;
                case ID_HELP_ABOUT:
                    MessageBoxA(
                        w_Handle,
                        "ChromaFast Palette Converter"
                        "\nCreated 2022 by Nitrocell (Baltazarus)"
                        "\n\nUse this tool to convert PAL to TPL format.",
                        "About",
                        MB_OK | MB_ICONINFORMATION
                    );
                    break;
            }
            break;
        }
        case WM_SIZE:
        {
            RECT wRect;
            GetClientRect(w_Handle, &wRect);
            int wrect_width = wRect.right - wRect.left;

            MoveWindow(w_EditPAL, 0, 0, wRect.right - wRect.left - 100, 25, TRUE);
            MoveWindow(w_ButtonConvert, 0, 25, wRect.right - wRect.left, 25, TRUE);

            RECT palRect;
            GetWindowRect(w_EditPAL, &palRect);
            int pal_edit_width = palRect.right - palRect.left;
            
            MoveWindow(w_ButtonBrowsePAL, pal_edit_width + 1, 0, wrect_width - pal_edit_width - 1, 25, TRUE);

            break;
        }
        case WM_GETMINMAXINFO:
        {
            POINT minReq = { 300, 92 + GetSystemMetrics(SM_CYMENU) };
            POINT maxReq = { 600, 92 + GetSystemMetrics(SM_CYMENU) };
            MINMAXINFO* pMinMaxInfo = reinterpret_cast<MINMAXINFO*>(lParam);
            pMinMaxInfo->ptMaxSize = minReq;
            pMinMaxInfo->ptMinTrackSize = minReq;  
            pMinMaxInfo->ptMaxTrackSize = maxReq;
            break;
        }
        case WM_CLOSE:
            DestroyWindow(w_Handle);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProcA(w_Handle, Msg, wParam, lParam);
    }
    return 0;
}

int __stdcall WinMain(HINSTANCE w_Inst, HINSTANCE w_PrevInst, char* lpCmdLine, int nCmdShow)
{
    WNDCLASSEXA wcex;
    HWND w_Handle = nullptr;
    
    memset(&wcex, 0, sizeof(wcex));
    wcex.cbSize = sizeof(WNDCLASSEXA);
    wcex.style = 0;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.lpfnWndProc = &WndProc;
    wcex.hInstance = GetModuleHandleA(nullptr);
    wcex.lpszClassName = LP_CLASS_NAME;
    wcex.lpszMenuName = MAKEINTRESOURCEA(IDR_MENUBAR);
    wcex.hIcon = LoadIcon(GetModuleHandleA(nullptr), IDI_APPLICATION);
    wcex.hCursor = LoadCursor(GetModuleHandleA(nullptr), IDC_ARROW);
    wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW);
    wcex.hIconSm = LoadIcon(GetModuleHandleA(nullptr), IDI_APPLICATION);

    if(!RegisterClassExA(&wcex))
    {
        MessageBoxA(nullptr, "Registration Failed!", "ERROR", MB_OK | MB_ICONEXCLAMATION);
        return -1;
    }

    w_Handle = CreateWindowExA(
        WS_EX_CLIENTEDGE,
        LP_CLASS_NAME,
        "ChromaFast PAL-TPL Converter",
        WS_OVERLAPPEDWINDOW,
        100, 100, 300, 128,
        nullptr, nullptr, GetModuleHandleA(nullptr), nullptr
    );

    if(w_Handle == nullptr)
    {
        MessageBoxA(nullptr, "Cannot create window!", "ERROR", MB_OK | MB_ICONEXCLAMATION);
        return -1;
    }

    ShowWindow(w_Handle, SW_SHOW);
    UpdateWindow(w_Handle);

    MSG Msg = { };
    while(GetMessageA(&Msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&Msg);
        DispatchMessageA(&Msg);
    }

    return 0;
}

void InitUI(HWND w_Handle)
{
    DWORD defStyles = (WS_VISIBLE | WS_CHILD);
    w_EditPAL = CreateWindowA(
        WC_EDITA, nullptr,
        defStyles | WS_BORDER | ES_AUTOHSCROLL,
        0, 0, 0, 0,
        w_Handle, nullptr, nullptr, nullptr
    );

    w_ButtonBrowsePAL = CreateWindowA(
        WC_BUTTONA, "Browse",
        defStyles,
        0, 0, 0, 0,
        w_Handle, reinterpret_cast<HMENU>(ID_BUTTON_PAL_BROWSE), nullptr, nullptr
    );

    w_ButtonConvert = CreateWindowA(
        WC_BUTTONA, "Convert",
        defStyles | BS_DEFPUSHBUTTON,
        0, 0, 0, 0,
        w_Handle, reinterpret_cast<HMENU>(ID_BUTTON_CONVERT), nullptr, nullptr
    );

    HFONT hFont = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    HWND w_Controls[] = {
        w_EditPAL, w_ButtonBrowsePAL, w_ButtonConvert
    };

    for(int i = 0; i < sizeof(w_Controls)/sizeof(HWND); ++i)
        SendMessageA(w_Controls[i], WM_SETFONT, reinterpret_cast<WPARAM>(hFont), 1u);

    return;
}

const char* OpenFilenameDlg(HWND w_Handle, bool bSave)
{
    static char path[MAX_PATH];

    OPENFILENAMEA ofn;

    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hInstance = GetModuleHandleA(nullptr);
    ofn.hwndOwner = w_Handle;
    ofn.lpstrFile = path;
    ofn.lpstrFile[0] = '\0';
    if(bSave)
    {
        ofn.lpstrFileTitle = const_cast<char*>("Save TPL Palette...");
        ofn.lpstrFilter = "TPL Palette\0*.tpl\0";
    }
    else
    {
        ofn.lpstrFileTitle = const_cast<char*>("Open PAL Palette");
        ofn.lpstrFilter = "PAL Palette\0*.pal\0";
    }
    ofn.nMaxFile = MAX_PATH;
    ofn.nFilterIndex = -1;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_EXPLORER;

    if(bSave)
        GetSaveFileNameA(&ofn);
    else
        GetOpenFileNameA(&ofn);

    return path;
}

std::pair<cf_byte,cf_byte> WordToBytes(cf_word color_bgr555)
{
    std::pair<cf_byte,cf_byte> result_pair;
    cf_byte hByte = ((cf_byte)(color_bgr555 >> 8));
    cf_byte lByte = (cf_byte)color_bgr555;
    result_pair.first = hByte;
    result_pair.second = lByte;
    return result_pair;
}

LRESULT __stdcall DlgProc_PaletteReview(HWND w_Dlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    ::w_DlgStaticReview = GetDlgItem(w_Dlg, IDC_STATIC_REVIEW);

    switch(Msg)
    {
        case WM_INITDIALOG:
        {
            RECT wRect;
            GetWindowRect(GetParent(w_Dlg), &wRect);
            SetWindowPos(w_Dlg, nullptr, wRect.left + 50, wRect.top + 50, 0, 0, SWP_NOSIZE);
            SetWindowSubclass(w_DlgStaticReview, (SUBCLASSPROC)SubClassProc_Review, 0u, 0u);
            break;
        }
        case WM_CLOSE:
        {
            RemoveWindowSubclass(w_DlgStaticReview, (SUBCLASSPROC)SubClassProc_Review, 0u);
            ReviewColorsVector.clear();
            EndDialog(w_Dlg, 0);
            break;
        }
    }
    return 0;
}

LRESULT __stdcall SubClassProc_Review(HWND w_Ctl, UINT Msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    switch(Msg)
    {
        case WM_PAINT:
        {
            PAINTSTRUCT ps = { };
            HDC hdc = BeginPaint(w_Ctl, &ps);
            
            RECT clrect;
            GetClientRect(w_Ctl, &clrect);

            int row = 0;
            int col = 0;
            RECT rct;
            rct.top = 16 * row;
            rct.left = 0;
            rct.right = (clrect.right - clrect.left) / 16;
            rct.bottom = 16;

            for(std::size_t i = 0ull; i < ReviewColorsVector.size(); ++i)
            {
                if((i % 16) == 0)
                {
                    rct.left = 0;
                    rct.top = 16 * row - 1;
                    row += 1;
                    col = 0;
                }

                rct.left = 16 * col;
                rct.right = 16 * col + 16;
                rct.bottom = 16 * (row - 1) + 16;

                HBRUSH hbr = CreateSolidBrush((COLORREF)(ReviewColorsVector[i]));

                FillRect(hdc, &rct, hbr);
                DeleteObject(hbr);
                col += 1;
            }
            EndPaint(w_Ctl, &ps);
            break;
        }
        default:
            return DefSubclassProc(w_Ctl, Msg, wParam, lParam);
    }
    return 0;
}