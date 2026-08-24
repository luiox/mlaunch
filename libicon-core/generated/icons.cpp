#include "icons.h"

#include <cstring>

namespace icon {

#if LIBICON_CORE_ENABLE_EMBED
static const char kSvg_Close[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"currentColor\"><path d=\"M11.9997 10.5865L16.9495 5.63672L18.3637 7.05093L13.4139 12.0007L18.3637 16.9504L16.9495 18.3646L11.9997 13.4149L7.04996 18.3646L5.63574 16.9504L10.5855 12.0007L5.63574 7.05093L7.04996 5.63672L11.9997 10.5865Z\"></path></svg>";
static const char kSvg_Minimize[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-minimize\"><path d=\"M8 3v3a2 2 0 0 1-2 2H3m18 0h-3a2 2 0 0 1-2-2V3m0 18v-3a2 2 0 0 1 2-2h3M3 16h3a2 2 0 0 1 2 2v3\"></path></svg>";
static const char kSvg_Maximize[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-maximize\"><path d=\"M8 3H5a2 2 0 0 0-2 2v3m18 0V5a2 2 0 0 0-2-2h-3m0 18h3a2 2 0 0 0 2-2v-3M3 16v3a2 2 0 0 0 2 2h3\"></path></svg>";
static const char kSvg_Info[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-info\"><circle cx=\"12\" cy=\"12\" r=\"10\"></circle><line x1=\"12\" y1=\"16\" x2=\"12\" y2=\"12\"></line><line x1=\"12\" y1=\"8\" x2=\"12.01\" y2=\"8\"></line></svg>";
static const char kSvg_Warning[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"currentColor\"><path d=\"M12 22C6.47715 22 2 17.5228 2 12C2 6.47715 6.47715 2 12 2C17.5228 2 22 6.47715 22 12C22 17.5228 17.5228 22 12 22ZM12 20C16.4183 20 20 16.4183 20 12C20 7.58172 16.4183 4 12 4C7.58172 4 4 7.58172 4 12C4 16.4183 7.58172 20 12 20ZM11 15H13V17H11V15ZM11 7H13V13H11V7Z\"></path></svg>";
static const char kSvg_Error[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"currentColor\"><path d=\"M12 22C6.47715 22 2 17.5228 2 12C2 6.47715 6.47715 2 12 2C17.5228 2 22 6.47715 22 12C22 17.5228 17.5228 22 12 22ZM11 15V17H13V15H11ZM11 7V13H13V7H11Z\"></path></svg>";
static const char kSvg_Settings[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-settings\"><circle cx=\"12\" cy=\"12\" r=\"3\"></circle><path d=\"M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 0 1 0 2.83 2 2 0 0 1-2.83 0l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-2 2 2 2 0 0 1-2-2v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 0 1-2.83 0 2 2 0 0 1 0-2.83l.06-.06a1.65 1.65 0 0 0 .33-1.82 1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1-2-2 2 2 0 0 1 2-2h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 0 1 0-2.83 2 2 0 0 1 2.83 0l.06.06a1.65 1.65 0 0 0 1.82.33H9a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 2-2 2 2 0 0 1 2 2v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 0 1 2.83 0 2 2 0 0 1 0 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 2 2 2 2 0 0 1-2 2h-.09a1.65 1.65 0 0 0-1.51 1z\"></path></svg>";
static const char kSvg_Menu[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-menu\"><line x1=\"3\" y1=\"12\" x2=\"21\" y2=\"12\"></line><line x1=\"3\" y1=\"6\" x2=\"21\" y2=\"6\"></line><line x1=\"3\" y1=\"18\" x2=\"21\" y2=\"18\"></line></svg>";
static const char kSvg_File[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-file\"><path d=\"M13 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V9z\"></path><polyline points=\"13 2 13 9 20 9\"></polyline></svg>";
static const char kSvg_Folder[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-folder\"><path d=\"M22 19a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5l2 3h9a2 2 0 0 1 2 2z\"></path></svg>";
static const char kSvg_FolderOpen[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"currentColor\"><path d=\"M3 21C2.44772 21 2 20.5523 2 20V4C2 3.44772 2.44772 3 3 3H10.4142L12.4142 5H20C20.5523 5 21 5.44772 21 6V9H19V7H11.5858L9.58579 5H4V16.998L5.5 11H22.5L20.1894 20.2425C20.0781 20.6877 19.6781 21 19.2192 21H3ZM19.9384 13H7.06155L5.56155 19H18.4384L19.9384 13Z\"></path></svg>";
static const char kSvg_Image[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-image\"><rect x=\"3\" y=\"3\" width=\"18\" height=\"18\" rx=\"2\" ry=\"2\"></rect><circle cx=\"8.5\" cy=\"8.5\" r=\"1.5\"></circle><polyline points=\"21 15 16 10 5 21\"></polyline></svg>";
static const char kSvg_Video[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-video\"><polygon points=\"23 7 16 12 23 17 23 7\"></polygon><rect x=\"1\" y=\"5\" width=\"15\" height=\"14\" rx=\"2\" ry=\"2\"></rect></svg>";
static const char kSvg_Archive[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-archive\"><polyline points=\"21 8 21 21 3 21 3 8\"></polyline><rect x=\"1\" y=\"3\" width=\"22\" height=\"5\"></rect><line x1=\"10\" y1=\"12\" x2=\"14\" y2=\"12\"></line></svg>";
static const char kSvg_Code[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-code\"><polyline points=\"16 18 22 12 16 6\"></polyline><polyline points=\"8 6 2 12 8 18\"></polyline></svg>";
static const char kSvg_New[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-plus\"><line x1=\"12\" y1=\"5\" x2=\"12\" y2=\"19\"></line><line x1=\"5\" y1=\"12\" x2=\"19\" y2=\"12\"></line></svg>";
static const char kSvg_Open[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-folder\"><path d=\"M22 19a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5l2 3h9a2 2 0 0 1 2 2z\"></path></svg>";
static const char kSvg_Save[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-save\"><path d=\"M19 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h11l5 5v11a2 2 0 0 1-2 2z\"></path><polyline points=\"17 21 17 13 7 13 7 21\"></polyline><polyline points=\"7 3 7 8 15 8\"></polyline></svg>";
static const char kSvg_Delete[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-delete\"><path d=\"M21 4H8l-7 8 7 8h13a2 2 0 0 0 2-2V6a2 2 0 0 0-2-2z\"></path><line x1=\"18\" y1=\"9\" x2=\"12\" y2=\"15\"></line><line x1=\"12\" y1=\"9\" x2=\"18\" y2=\"15\"></line></svg>";
static const char kSvg_Edit[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-edit\"><path d=\"M11 4H4a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h14a2 2 0 0 0 2-2v-7\"></path><path d=\"M18.5 2.5a2.121 2.121 0 0 1 3 3L12 15l-4 1 1-4 9.5-9.5z\"></path></svg>";
static const char kSvg_Copy[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-copy\"><rect x=\"9\" y=\"9\" width=\"13\" height=\"13\" rx=\"2\" ry=\"2\"></rect><path d=\"M5 15H4a2 2 0 0 1-2-2V4a2 2 0 0 1 2-2h9a2 2 0 0 1 2 2v1\"></path></svg>";
static const char kSvg_Undo[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"currentColor\"><path d=\"M5.82843 6.99955L8.36396 9.53509L6.94975 10.9493L2 5.99955L6.94975 1.0498L8.36396 2.46402L5.82843 4.99955H13C17.4183 4.99955 21 8.58127 21 12.9996C21 17.4178 17.4183 20.9996 13 20.9996H4V18.9996H13C16.3137 18.9996 19 16.3133 19 12.9996C19 9.68584 16.3137 6.99955 13 6.99955H5.82843Z\"></path></svg>";
static const char kSvg_Redo[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"currentColor\"><path d=\"M18.1716 6.99955H11C7.68629 6.99955 5 9.68584 5 12.9996C5 16.3133 7.68629 18.9996 11 18.9996H20V20.9996H11C6.58172 20.9996 3 17.4178 3 12.9996C3 8.58127 6.58172 4.99955 11 4.99955H18.1716L15.636 2.46402L17.0503 1.0498L22 5.99955L17.0503 10.9493L15.636 9.53509L18.1716 6.99955Z\"></path></svg>";
static const char kSvg_Find[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-search\"><circle cx=\"11\" cy=\"11\" r=\"8\"></circle><line x1=\"21\" y1=\"21\" x2=\"16.65\" y2=\"16.65\"></line></svg>";
static const char kSvg_Replace[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-refresh-cw\"><polyline points=\"23 4 23 10 17 10\"></polyline><polyline points=\"1 20 1 14 7 14\"></polyline><path d=\"M3.51 9a9 9 0 0 1 14.85-3.36L23 10M1 14l4.64 4.36A9 9 0 0 0 20.49 15\"></path></svg>";
static const char kSvg_Back[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-arrow-left\"><line x1=\"19\" y1=\"12\" x2=\"5\" y2=\"12\"></line><polyline points=\"12 19 5 12 12 5\"></polyline></svg>";
static const char kSvg_Forward[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-arrow-right\"><line x1=\"5\" y1=\"12\" x2=\"19\" y2=\"12\"></line><polyline points=\"12 5 19 12 12 19\"></polyline></svg>";
static const char kSvg_Up[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-arrow-up\"><line x1=\"12\" y1=\"19\" x2=\"12\" y2=\"5\"></line><polyline points=\"5 12 12 5 19 12\"></polyline></svg>";
static const char kSvg_Refresh[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-refresh-cw\"><polyline points=\"23 4 23 10 17 10\"></polyline><polyline points=\"1 20 1 14 7 14\"></polyline><path d=\"M3.51 9a9 9 0 0 1 14.85-3.36L23 10M1 14l4.64 4.36A9 9 0 0 0 20.49 15\"></path></svg>";
static const char kSvg_Home[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-home\"><path d=\"M3 9l9-7 9 7v11a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z\"></path><polyline points=\"9 22 9 12 15 12 15 22\"></polyline></svg>";
static const char kSvg_ZoomIn[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-zoom-in\"><circle cx=\"11\" cy=\"11\" r=\"8\"></circle><line x1=\"21\" y1=\"21\" x2=\"16.65\" y2=\"16.65\"></line><line x1=\"11\" y1=\"8\" x2=\"11\" y2=\"14\"></line><line x1=\"8\" y1=\"11\" x2=\"14\" y2=\"11\"></line></svg>";
static const char kSvg_ZoomOut[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-zoom-out\"><circle cx=\"11\" cy=\"11\" r=\"8\"></circle><line x1=\"21\" y1=\"21\" x2=\"16.65\" y2=\"16.65\"></line><line x1=\"8\" y1=\"11\" x2=\"14\" y2=\"11\"></line></svg>";
static const char kSvg_Fullscreen[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-maximize\"><path d=\"M8 3H5a2 2 0 0 0-2 2v3m18 0V5a2 2 0 0 0-2-2h-3m0 18h3a2 2 0 0 0 2-2v-3M3 16v3a2 2 0 0 0 2 2h3\"></path></svg>";
static const char kSvg_FullscreenExit[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-minimize\"><path d=\"M8 3v3a2 2 0 0 1-2 2H3m18 0h-3a2 2 0 0 1-2-2V3m0 18v-3a2 2 0 0 1 2-2h3M3 16h3a2 2 0 0 1 2 2v3\"></path></svg>";
static const char kSvg_Grid[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-grid\"><rect x=\"3\" y=\"3\" width=\"7\" height=\"7\"></rect><rect x=\"14\" y=\"3\" width=\"7\" height=\"7\"></rect><rect x=\"14\" y=\"14\" width=\"7\" height=\"7\"></rect><rect x=\"3\" y=\"14\" width=\"7\" height=\"7\"></rect></svg>";
static const char kSvg_Search[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-search\"><circle cx=\"11\" cy=\"11\" r=\"8\"></circle><line x1=\"21\" y1=\"21\" x2=\"16.65\" y2=\"16.65\"></line></svg>";
static const char kSvg_Filter[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-filter\"><polygon points=\"22 3 2 3 10 12.46 10 19 14 21 14 12.46 22 3\"></polygon></svg>";
static const char kSvg_SortAsc[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-arrow-up\"><line x1=\"12\" y1=\"19\" x2=\"12\" y2=\"5\"></line><polyline points=\"5 12 12 5 19 12\"></polyline></svg>";
static const char kSvg_SortDesc[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-arrow-down\"><line x1=\"12\" y1=\"5\" x2=\"12\" y2=\"19\"></line><polyline points=\"19 12 12 19 5 12\"></polyline></svg>";
static const char kSvg_Check[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-check\"><polyline points=\"20 6 9 17 4 12\"></polyline></svg>";
static const char kSvg_Circle[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-circle\"><circle cx=\"12\" cy=\"12\" r=\"10\"></circle></svg>";
static const char kSvg_Square[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-square\"><rect x=\"3\" y=\"3\" width=\"18\" height=\"18\" rx=\"2\" ry=\"2\"></rect></svg>";
static const char kSvg_Star[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-star\"><polygon points=\"12 2 15.09 8.26 22 9.27 17 14.14 18.18 21.02 12 17.77 5.82 21.02 7 14.14 2 9.27 8.91 8.26 12 2\"></polygon></svg>";
static const char kSvg_Heart[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-heart\"><path d=\"M20.84 4.61a5.5 5.5 0 0 0-7.78 0L12 5.67l-1.06-1.06a5.5 5.5 0 0 0-7.78 7.78l1.06 1.06L12 21.23l7.78-7.78 1.06-1.06a5.5 5.5 0 0 0 0-7.78z\"></path></svg>";
static const char kSvg_Flag[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-flag\"><path d=\"M4 15s1-1 4-1 5 2 8 2 4-1 4-1V3s-1 1-4 1-5-2-8-2-4 1-4 1z\"></path><line x1=\"4\" y1=\"22\" x2=\"4\" y2=\"15\"></line></svg>";
static const char kSvg_Tag[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-tag\"><path d=\"M20.59 13.41l-7.17 7.17a2 2 0 0 1-2.83 0L2 12V2h10l8.59 8.59a2 2 0 0 1 0 2.82z\"></path><line x1=\"7\" y1=\"7\" x2=\"7.01\" y2=\"7\"></line></svg>";
static const char kSvg_Clock[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-clock\"><circle cx=\"12\" cy=\"12\" r=\"10\"></circle><polyline points=\"12 6 12 12 16 14\"></polyline></svg>";
static const char kSvg_Calendar[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-calendar\"><rect x=\"3\" y=\"4\" width=\"18\" height=\"18\" rx=\"2\" ry=\"2\"></rect><line x1=\"16\" y1=\"2\" x2=\"16\" y2=\"6\"></line><line x1=\"8\" y1=\"2\" x2=\"8\" y2=\"6\"></line><line x1=\"3\" y1=\"10\" x2=\"21\" y2=\"10\"></line></svg>";
static const char kSvg_Link[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-link\"><path d=\"M10 13a5 5 0 0 0 7.54.54l3-3a5 5 0 0 0-7.07-7.07l-1.72 1.71\"></path><path d=\"M14 11a5 5 0 0 0-7.54-.54l-3 3a5 5 0 0 0 7.07 7.07l1.71-1.71\"></path></svg>";
static const char kSvg_Web[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-globe\"><circle cx=\"12\" cy=\"12\" r=\"10\"></circle><line x1=\"2\" y1=\"12\" x2=\"22\" y2=\"12\"></line><path d=\"M12 2a15.3 15.3 0 0 1 4 10 15.3 15.3 0 0 1-4 10 15.3 15.3 0 0 1-4-10 15.3 15.3 0 0 1 4-10z\"></path></svg>";
static const char kSvg_Download[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-download\"><path d=\"M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4\"></path><polyline points=\"7 10 12 15 17 10\"></polyline><line x1=\"12\" y1=\"15\" x2=\"12\" y2=\"3\"></line></svg>";
static const char kSvg_Upload[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-upload\"><path d=\"M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4\"></path><polyline points=\"17 8 12 3 7 8\"></polyline><line x1=\"12\" y1=\"3\" x2=\"12\" y2=\"15\"></line></svg>";
static const char kSvg_Cloud[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-cloud\"><path d=\"M18 10h-1.26A8 8 0 1 0 9 20h9a5 5 0 0 0 0-10z\"></path></svg>";
static const char kSvg_Share[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-share\"><path d=\"M4 12v8a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2v-8\"></path><polyline points=\"16 6 12 2 8 6\"></polyline><line x1=\"12\" y1=\"2\" x2=\"12\" y2=\"15\"></line></svg>";
static const char kSvg_External[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-external-link\"><path d=\"M18 13v6a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V8a2 2 0 0 1 2-2h6\"></path><polyline points=\"15 3 21 3 21 9\"></polyline><line x1=\"10\" y1=\"14\" x2=\"21\" y2=\"3\"></line></svg>";
static const char kSvg_Play[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-play\"><polygon points=\"5 3 19 12 5 21 5 3\"></polygon></svg>";
static const char kSvg_Pause[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-pause\"><rect x=\"6\" y=\"4\" width=\"4\" height=\"16\"></rect><rect x=\"14\" y=\"4\" width=\"4\" height=\"16\"></rect></svg>";
static const char kSvg_Prev[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-skip-back\"><polygon points=\"19 20 9 12 19 4 19 20\"></polygon><line x1=\"5\" y1=\"19\" x2=\"5\" y2=\"5\"></line></svg>";
static const char kSvg_Next[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-skip-forward\"><polygon points=\"5 4 15 12 5 20 5 4\"></polygon><line x1=\"19\" y1=\"5\" x2=\"19\" y2=\"19\"></line></svg>";
static const char kSvg_Volume[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-volume\"><polygon points=\"11 5 6 9 2 9 2 15 6 15 11 19 11 5\"></polygon></svg>";
static const char kSvg_VolumeMute[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-volume-x\"><polygon points=\"11 5 6 9 2 9 2 15 6 15 11 19 11 5\"></polygon><line x1=\"23\" y1=\"9\" x2=\"17\" y2=\"15\"></line><line x1=\"17\" y1=\"9\" x2=\"23\" y2=\"15\"></line></svg>";
static const char kSvg_Mic[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-mic\"><path d=\"M12 1a3 3 0 0 0-3 3v8a3 3 0 0 0 6 0V4a3 3 0 0 0-3-3z\"></path><path d=\"M19 10v2a7 7 0 0 1-14 0v-2\"></path><line x1=\"12\" y1=\"19\" x2=\"12\" y2=\"23\"></line><line x1=\"8\" y1=\"23\" x2=\"16\" y2=\"23\"></line></svg>";
static const char kSvg_Computer[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-monitor\"><rect x=\"2\" y=\"3\" width=\"20\" height=\"14\" rx=\"2\" ry=\"2\"></rect><line x1=\"8\" y1=\"21\" x2=\"16\" y2=\"21\"></line><line x1=\"12\" y1=\"17\" x2=\"12\" y2=\"21\"></line></svg>";
static const char kSvg_Cmd[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-terminal\"><polyline points=\"4 17 10 11 4 5\"></polyline><line x1=\"12\" y1=\"19\" x2=\"20\" y2=\"19\"></line></svg>";
static const char kSvg_Shutdown[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-power\"><path d=\"M18.36 6.64a9 9 0 1 1-12.73 0\"></path><line x1=\"12\" y1=\"2\" x2=\"12\" y2=\"12\"></line></svg>";
static const char kSvg_Restart[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-rotate-cw\"><polyline points=\"23 4 23 10 17 10\"></polyline><path d=\"M20.49 15a9 9 0 1 1-2.12-9.36L23 10\"></path></svg>";
static const char kSvg_Lock[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-lock\"><rect x=\"3\" y=\"11\" width=\"18\" height=\"11\" rx=\"2\" ry=\"2\"></rect><path d=\"M7 11V7a5 5 0 0 1 10 0v4\"></path></svg>";
static const char kSvg_Unlock[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-unlock\"><rect x=\"3\" y=\"11\" width=\"18\" height=\"11\" rx=\"2\" ry=\"2\"></rect><path d=\"M7 11V7a5 5 0 0 1 9.9-1\"></path></svg>";
static const char kSvg_Key[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-key\"><path d=\"M21 2l-2 2m-7.61 7.61a5.5 5.5 0 1 1-7.778 7.778 5.5 5.5 0 0 1 7.777-7.777zm0 0L15.5 7.5m0 0l3 3L22 7l-3-3m-3.5 3.5L19 4\"></path></svg>";
static const char kSvg_Printer[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-printer\"><polyline points=\"6 9 6 2 18 2 18 9\"></polyline><path d=\"M6 18H4a2 2 0 0 1-2-2v-5a2 2 0 0 1 2-2h16a2 2 0 0 1 2 2v5a2 2 0 0 1-2 2h-2\"></path><rect x=\"6\" y=\"14\" width=\"12\" height=\"8\"></rect></svg>";
static const char kSvg_Battery[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-battery\"><rect x=\"1\" y=\"6\" width=\"18\" height=\"12\" rx=\"2\" ry=\"2\"></rect><line x1=\"23\" y1=\"13\" x2=\"23\" y2=\"11\"></line></svg>";
static const char kSvg_Wifi[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-wifi\"><path d=\"M5 12.55a11 11 0 0 1 14.08 0\"></path><path d=\"M1.42 9a16 16 0 0 1 21.16 0\"></path><path d=\"M8.53 16.11a6 6 0 0 1 6.95 0\"></path><line x1=\"12\" y1=\"20\" x2=\"12.01\" y2=\"20\"></line></svg>";
static const char kSvg_App[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-box\"><path d=\"M21 16V8a2 2 0 0 0-1-1.73l-7-4a2 2 0 0 0-2 0l-7 4A2 2 0 0 0 3 8v8a2 2 0 0 0 1 1.73l7 4a2 2 0 0 0 2 0l7-4A2 2 0 0 0 21 16z\"></path><polyline points=\"3.27 6.96 12 12.01 20.73 6.96\"></polyline><line x1=\"12\" y1=\"22.08\" x2=\"12\" y2=\"12\"></line></svg>";
static const char kSvg_Pin[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"currentColor\"><path d=\"M13.8273 1.69L22.3126 10.1753L20.8984 11.5895L20.1913 10.8824L15.9486 15.125L15.2415 18.6606L13.8273 20.0748L9.58466 15.8321L4.63492 20.7819L3.2207 19.3677L8.17045 14.4179L3.92781 10.1753L5.34202 8.76107L8.87756 8.05396L13.1202 3.81132L12.4131 3.10422L13.8273 1.69ZM14.5344 5.22554L9.86358 9.89637L7.0417 10.4607L13.5418 16.9609L14.1062 14.139L18.7771 9.46818L14.5344 5.22554Z\"></path></svg>";
static const char kSvg_Unpin[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 24 24\" fill=\"currentColor\"><path d=\"M18 3V5H17V11L19 14V16H13V23H11V16H5V14L7 11V5H6V3H18ZM9 5V11.6056L7.4037 14H16.5963L15 11.6056V5H9Z\"></path></svg>";
static const char kSvg_Plus[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-plus\"><line x1=\"12\" y1=\"5\" x2=\"12\" y2=\"19\"></line><line x1=\"5\" y1=\"12\" x2=\"19\" y2=\"12\"></line></svg>";
static const char kSvg_Minus[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-minus\"><line x1=\"5\" y1=\"12\" x2=\"19\" y2=\"12\"></line></svg>";
static const char kSvg_History[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-clock\"><circle cx=\"12\" cy=\"12\" r=\"10\"></circle><polyline points=\"12 6 12 12 16 14\"></polyline></svg>";
static const char kSvg_Clear[] = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" class=\"feather feather-x-circle\"><circle cx=\"12\" cy=\"12\" r=\"10\"></circle><line x1=\"15\" y1=\"9\" x2=\"9\" y2=\"15\"></line><line x1=\"9\" y1=\"9\" x2=\"15\" y2=\"15\"></line></svg>";
static const char kSvg_ANT_ThemeSun[] = "<?xml version=\"1.0\" standalone=\"no\"?><!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\"><svg t=\"1754763265594\" class=\"icon\" viewBox=\"0 0 1024 1024\" version=\"1.1\" xmlns=\"http://www.w3.org/2000/svg\" p-id=\"9791\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\"256\" height=\"256\"><path d=\"M512 853.333333a42.666667 42.666667 0 0 1 42.666667 42.666667v85.333333a42.666667 42.666667 0 0 1-85.333334 0v-85.333333a42.666667 42.666667 0 0 1 42.666667-42.666667z m301.696-99.968l60.330667 60.330667a42.666667 42.666667 0 0 1-60.330667 60.330667l-60.330667-60.330667a42.666667 42.666667 0 0 1 60.330667-60.330667z m-543.061333 0a42.666667 42.666667 0 0 1 0 60.330667L210.346667 874.026667a42.666667 42.666667 0 1 1-60.330667-60.330667l60.330667-60.330667a42.666667 42.666667 0 0 1 60.330666 0zM512 256a256 256 0 1 1 0 512 256 256 0 0 1 0-512z m0 85.333333a170.112 170.112 0 0 0-120.661333 50.005334A170.112 170.112 0 0 0 341.333333 512c0 47.146667 19.114667 89.813333 50.005334 120.661333A170.112 170.112 0 0 0 512 682.666667a170.112 170.112 0 0 0 120.661333-50.005334A170.112 170.112 0 0 0 682.666667 512a170.112 170.112 0 0 0-50.005334-120.661333A170.112 170.112 0 0 0 512 341.333333z m469.333333 128a42.666667 42.666667 0 0 1 0 85.333334h-85.333333a42.666667 42.666667 0 0 1 0-85.333334h85.333333zM128 469.333333a42.666667 42.666667 0 0 1 0 85.333334H42.666667a42.666667 42.666667 0 0 1 0-85.333334h85.333333z m746.026667-319.36a42.666667 42.666667 0 0 1 0 60.330667l-60.330667 60.330667a42.666667 42.666667 0 0 1-60.330667-60.330667l60.330667-60.330667a42.666667 42.666667 0 0 1 60.330667 0z m-663.722667 0l60.330667 60.330667A42.666667 42.666667 0 0 1 210.346667 270.634667L149.973333 210.346667a42.666667 42.666667 0 1 1 60.330667-60.330667zM512 0a42.666667 42.666667 0 0 1 42.666667 42.666667v85.333333a42.666667 42.666667 0 0 1-85.333334 0V42.666667a42.666667 42.666667 0 0 1 42.666667-42.666667z\" fill=\"#8a8a8a\" p-id=\"9792\"></path></svg>";
static const char kSvg_ANT_ThemeMoon[] = "<?xml version=\"1.0\" standalone=\"no\"?><!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\"><svg t=\"1754763333105\" class=\"icon\" viewBox=\"0 0 1024 1024\" version=\"1.1\" xmlns=\"http://www.w3.org/2000/svg\" p-id=\"3907\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\"256\" height=\"256\"><path d=\"M512 85.333333l10.709333 0.213334a12.288 12.288 0 0 1 9.429334 19.370666c-22.656 31.914667-36.48 53.76-41.514667 65.621334a277.333333 277.333333 0 0 0 350.250667 367.701333l11.776-4.522667c15.701333-6.528 35.370667-18.346667 59.008-35.498666A17.024 17.024 0 0 1 938.666667 512c0 9.6-0.298667 19.157333-0.938667 28.586667l-1.152 14.08-0.853333 7.68-1.706667 12.885333C903.466667 780.885333 726.144 938.666667 512 938.666667 276.352 938.666667 85.333333 747.648 85.333333 512c0-210.218667 152.064-384.938667 352.170667-420.181333l14.378667-2.261334 10.112-1.322666 7.338666-0.810667c14.037333-1.365333 28.245333-2.090667 42.666667-2.090667zM394.24 191.530667l-7.125333 2.688A341.461333 341.461333 0 0 0 512 853.333333a341.461333 341.461333 0 0 0 320.469333-223.616A362.666667 362.666667 0 0 1 394.24 191.530667z\" fill=\"#8a8a8a\" p-id=\"3908\"></path></svg>";
static const char kSvg_ANT_WindowMin[] = "<?xml version=\"1.0\" standalone=\"no\"?><!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\"><svg t=\"1749639675580\" class=\"icon\" viewBox=\"0 0 1024 1024\" version=\"1.1\" xmlns=\"http://www.w3.org/2000/svg\" p-id=\"7150\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\"256\" height=\"256\"><path d=\"M923 571H130.7c-27.6 0-50-22.4-50-50s22.4-50 50-50H923c27.6 0 50 22.4 50 50s-22.4 50-50 50z\" fill=\"#6B6B6B\" p-id=\"7151\"></path></svg>";
static const char kSvg_ANT_WindowMax[] = "<?xml version=\"1.0\" standalone=\"no\"?><!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\"><svg t=\"1749639556782\" class=\"icon\" viewBox=\"0 0 1024 1024\" version=\"1.1\" xmlns=\"http://www.w3.org/2000/svg\" p-id=\"6087\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\"256\" height=\"256\"><path d=\"M812.3 959.4H213.7c-81.6 0-148-66.4-148-148V212.9c0-81.6 66.4-148 148-148h598.5c81.6 0 148 66.4 148 148v598.5C960.3 893 893.9 959.4 812.3 959.4zM213.7 120.9c-50.7 0-92 41.3-92 92v598.5c0 50.7 41.3 92 92 92h598.5c50.7 0 92-41.3 92-92V212.9c0-50.7-41.3-92-92-92H213.7z\" fill=\"#6B6B6B\" p-id=\"6088\"></path></svg>";
static const char kSvg_ANT_WindowClose[] = "<?xml version=\"1.0\" standalone=\"no\"?><!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\"><svg t=\"1749639638960\" class=\"icon\" viewBox=\"0 0 1024 1024\" version=\"1.1\" xmlns=\"http://www.w3.org/2000/svg\" p-id=\"6729\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\"256\" height=\"256\"><path d=\"M109.9 935.8c-19.5-19.5-19.5-51.2 0-70.7l759.3-759.3c19.5-19.5 51.2-19.5 70.7 0s19.5 51.2 0 70.7L180.6 935.8c-19.6 19.6-51.2 19.6-70.7 0z\" fill=\"#6B6B6B\" p-id=\"6730\"></path><path d=\"M869.1 935.8L109.9 176.5c-19.5-19.5-19.5-51.2 0-70.7s51.2-19.5 70.7 0l759.3 759.3c19.5 19.5 19.5 51.2 0 70.7-19.6 19.6-51.2 19.6-70.8 0z\" fill=\"#6B6B6B\" p-id=\"6731\"></path></svg>";
static const char kSvg_ANT_WindowRestore[] = "<?xml version=\"1.0\" standalone=\"no\"?><!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\"><svg t=\"1749639645750\" class=\"icon\" viewBox=\"0 0 1024 1024\" version=\"1.1\" xmlns=\"http://www.w3.org/2000/svg\" p-id=\"6940\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\"256\" height=\"256\"><path d=\"M812.2 65H351.6c-78.3 0-142.5 61.1-147.7 138.1-77 5.1-138.1 69.4-138.1 147.7v460.6c0 81.6 66.4 148 148 148h460.6c78.3 0 142.5-61.1 147.7-138.1 77-5.1 138.1-69.4 138.1-147.7V213c0-81.6-66.4-148-148-148z m-45.8 746.3c0 50.7-41.3 92-92 92H213.8c-50.7 0-92-41.3-92-92V350.7c0-50.7 41.3-92 92-92h460.6c50.7 0 92 41.3 92 92v460.6z m137.8-137.7c0 47.3-35.8 86.3-81.8 91.4V350.7c0-81.6-66.4-148-148-148H260.2c5.1-45.9 44.2-81.8 91.4-81.8h460.6c50.7 0 92 41.3 92 92v460.7z\" fill=\"#6B6B6B\" p-id=\"6941\"></path></svg>";
static const char kSvg_ANT_NavHome[] = "<?xml version=\"1.0\" standalone=\"no\"?><!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\"><svg t=\"1749463030047\" class=\"icon\" viewBox=\"0 0 1024 1024\" version=\"1.1\" xmlns=\"http://www.w3.org/2000/svg\" p-id=\"9029\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\"256\" height=\"256\"><path d=\"M549.61981 133.022476l319.683047 203.605334A70.851048 70.851048 0 0 1 902.095238 396.361143v434.883047A70.89981 70.89981 0 0 1 831.146667 902.095238l-248.222477-0.024381V617.910857h-153.673142v284.16L192.828952 902.095238A70.89981 70.89981 0 0 1 121.904762 831.24419V390.241524c0-24.527238 12.678095-47.299048 33.54819-60.220953l318.659048-197.485714a70.972952 70.972952 0 0 1 75.50781 0.487619z\" p-id=\"9030\" fill=\"#bfbfbf\"></path></svg>";
static const char kSvg_ANT_NavHomeActive[] = "<?xml version=\"1.0\" standalone=\"no\"?><!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\"><svg t=\"1749471876567\" class=\"icon\" viewBox=\"0 0 1024 1024\" version=\"1.1\" xmlns=\"http://www.w3.org/2000/svg\" p-id=\"9237\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\"256\" height=\"256\"><path d=\"M549.61981 133.022476l319.683047 203.605334A70.851048 70.851048 0 0 1 902.095238 396.361143v434.883047A70.89981 70.89981 0 0 1 831.146667 902.095238l-248.222477-0.024381V617.910857h-153.673142v284.16L192.828952 902.095238A70.89981 70.89981 0 0 1 121.904762 831.24419V390.241524c0-24.527238 12.678095-47.299048 33.54819-60.220953l318.659048-197.485714a70.972952 70.972952 0 0 1 75.50781 0.487619z\" p-id=\"9238\" fill=\"#40A9FF\"></path></svg>";
static const char kSvg_ANT_NavFunction[] = "<?xml version=\"1.0\" standalone=\"no\"?><!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\"><svg t=\"1755000647699\" class=\"icon\" viewBox=\"0 0 1024 1024\" version=\"1.1\" xmlns=\"http://www.w3.org/2000/svg\" p-id=\"19178\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\"256\" height=\"256\"><path d=\"M484.693333 397.91616c0 50.752853-41.147733 91.897173-91.897173 91.897173H218.190507C167.441067 489.813333 126.293333 448.6656 126.293333 397.91616V223.310507C126.293333 172.561067 167.441067 131.413333 218.190507 131.413333h174.60224C443.5456 131.413333 484.693333 172.561067 484.693333 223.310507v174.605653zM884.053333 797.272747c0 50.752853-41.147733 91.900587-91.900586 91.900586h-174.598827c-50.752853 0-91.900587-41.147733-91.900587-91.900586v-174.598827c0-50.752853 41.147733-91.900587 91.900587-91.900587h174.598827c50.752853 0 91.900587 41.147733 91.900586 91.900587v174.598827zM484.481707 797.272747c0 50.752853-41.147733 91.900587-91.897174 91.900586H217.97888c-50.752853 0-91.897173-41.147733-91.897173-91.900586v-174.598827c0-50.752853 41.147733-91.900587 91.897173-91.900587h174.60224c50.752853 0 91.897173 41.147733 91.897173 91.900587v174.598827zM782.81728 211.27168a21.189973 21.189973 0 0 1 21.16608 21.162667v156.357973c0 11.666773-9.495893 21.162667-21.16608 21.162667h-156.35456a21.189973 21.189973 0 0 1-21.16608-21.162667V232.434347c0-11.666773 9.495893-21.162667 21.16608-21.162667h156.35456m9.12384-79.858347h-174.598827c-50.752853 0-91.900587 41.147733-91.900586 91.900587v174.598827c0 50.752853 41.147733 91.900587 91.900586 91.900586h174.598827c50.752853 0 91.900587-41.147733 91.900587-91.900586V223.31392c0-50.752853-41.147733-91.900587-91.900587-91.900587z\" fill=\"#bfbfbf\" p-id=\"19179\"></path></svg>";
static const char kSvg_ANT_NavFunctionActive[] = "<?xml version=\"1.0\" standalone=\"no\"?><!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\"><svg t=\"1755000647699\" class=\"icon\" viewBox=\"0 0 1024 1024\" version=\"1.1\" xmlns=\"http://www.w3.org/2000/svg\" p-id=\"19178\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\"256\" height=\"256\"><path d=\"M484.693333 397.91616c0 50.752853-41.147733 91.897173-91.897173 91.897173H218.190507C167.441067 489.813333 126.293333 448.6656 126.293333 397.91616V223.310507C126.293333 172.561067 167.441067 131.413333 218.190507 131.413333h174.60224C443.5456 131.413333 484.693333 172.561067 484.693333 223.310507v174.605653zM884.053333 797.272747c0 50.752853-41.147733 91.900587-91.900586 91.900586h-174.598827c-50.752853 0-91.900587-41.147733-91.900587-91.900586v-174.598827c0-50.752853 41.147733-91.900587 91.900587-91.900587h174.598827c50.752853 0 91.900587 41.147733 91.900586 91.900587v174.598827zM484.481707 797.272747c0 50.752853-41.147733 91.900587-91.897174 91.900586H217.97888c-50.752853 0-91.897173-41.147733-91.897173-91.900586v-174.598827c0-50.752853 41.147733-91.900587 91.897173-91.900587h174.60224c50.752853 0 91.897173 41.147733 91.897173 91.900587v174.598827zM782.81728 211.27168a21.189973 21.189973 0 0 1 21.16608 21.162667v156.357973c0 11.666773-9.495893 21.162667-21.16608 21.162667h-156.35456a21.189973 21.189973 0 0 1-21.16608-21.162667V232.434347c0-11.666773 9.495893-21.162667 21.16608-21.162667h156.35456m9.12384-79.858347h-174.598827c-50.752853 0-91.900587 41.147733-91.900586 91.900587v174.598827c0 50.752853 41.147733 91.900587 91.900586 91.900586h174.598827c50.752853 0 91.900587-41.147733 91.900587-91.900586V223.31392c0-50.752853-41.147733-91.900587-91.900587-91.900587z\" fill=\"#40A9FF\" p-id=\"19179\"></path></svg>";
static const char kSvg_ANT_NavSettings[] = "<?xml version=\"1.0\" standalone=\"no\"?><!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\"><svg t=\"1749463016570\" class=\"icon\" viewBox=\"0 0 1024 1024\" version=\"1.1\" xmlns=\"http://www.w3.org/2000/svg\" p-id=\"8807\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\"256\" height=\"256\"><path d=\"M448.487619 97.52381l130.096762 0.170666c40.399238 0.073143 73.142857 32.792381 73.191619 73.216l0.048762 21.211429a345.283048 345.283048 0 0 1 71.143619 39.960381l17.408-10.044953a73.313524 73.313524 0 0 1 99.961905 26.819048l65.219047 112.566857a73.313524 73.313524 0 0 1-22.893714 97.816381l-3.974095 2.438095-17.481143 10.093715a341.479619 341.479619 0 0 1-1.292191 83.968l12.361143 7.168a73.313524 73.313524 0 0 1 28.867048 96.329142l-2.023619 3.803429-61.098667 105.813333a73.313524 73.313524 0 0 1-96.329143 28.867048l-3.803428-2.048-16.896-9.752381a341.918476 341.918476 0 0 1-68.291048 38.083048l0.024381 29.062095a73.313524 73.313524 0 0 1-68.754286 73.264762l-4.632381 0.146285-130.121142-0.170666a73.313524 73.313524 0 0 1-73.191619-73.216l-0.048762-35.035429a346.599619 346.599619 0 0 1-57.368381-34.035809l-31.158857 17.944381a73.313524 73.313524 0 0 1-99.986286-26.819048l-65.219048-112.566857a73.313524 73.313524 0 0 1 22.918095-97.816381l3.949715-2.438095 31.719619-18.285715c-2.438095-23.161905-2.56-46.665143-0.219429-70.119619l-35.206095-20.333714a73.313524 73.313524 0 0 1-28.891429-96.329143l2.048-3.803428 61.098667-105.813334a73.313524 73.313524 0 0 1 96.329143-28.867047l3.803429 2.048 30.72 17.724952a341.284571 341.284571 0 0 1 64.609523-39.716571l-0.048762-27.89181a73.313524 73.313524 0 0 1 68.754286-73.264762L448.487619 97.52381z m145.798095 283.721142a146.407619 146.407619 0 0 0-200.167619 53.638096 146.773333 146.773333 0 0 0 53.711238 200.362666 146.407619 146.407619 0 0 0 200.167619-53.638095 146.773333 146.773333 0 0 0-53.711238-200.362667z m-136.655238 90.258286a73.118476 73.118476 0 0 1 96.182857-28.842667l3.803429 2.048 3.657143 2.267429a73.508571 73.508571 0 0 1 23.210666 98.011429 73.118476 73.118476 0 0 1-99.961904 26.819047 73.48419 73.48419 0 0 1-26.892191-100.303238z\" p-id=\"8808\" fill=\"#bfbfbf\"></path></svg>";
static const char kSvg_ANT_NavSettingsActive[] = "<?xml version=\"1.0\" standalone=\"no\"?><!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\"><svg t=\"1749471864450\" class=\"icon\" viewBox=\"0 0 1024 1024\" version=\"1.1\" xmlns=\"http://www.w3.org/2000/svg\" p-id=\"9015\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\"256\" height=\"256\"><path d=\"M448.487619 97.52381l130.096762 0.170666c40.399238 0.073143 73.142857 32.792381 73.191619 73.216l0.048762 21.211429a345.283048 345.283048 0 0 1 71.143619 39.960381l17.408-10.044953a73.313524 73.313524 0 0 1 99.961905 26.819048l65.219047 112.566857a73.313524 73.313524 0 0 1-22.893714 97.816381l-3.974095 2.438095-17.481143 10.093715a341.479619 341.479619 0 0 1-1.292191 83.968l12.361143 7.168a73.313524 73.313524 0 0 1 28.867048 96.329142l-2.023619 3.803429-61.098667 105.813333a73.313524 73.313524 0 0 1-96.329143 28.867048l-3.803428-2.048-16.896-9.752381a341.918476 341.918476 0 0 1-68.291048 38.083048l0.024381 29.062095a73.313524 73.313524 0 0 1-68.754286 73.264762l-4.632381 0.146285-130.121142-0.170666a73.313524 73.313524 0 0 1-73.191619-73.216l-0.048762-35.035429a346.599619 346.599619 0 0 1-57.368381-34.035809l-31.158857 17.944381a73.313524 73.313524 0 0 1-99.986286-26.819048l-65.219048-112.566857a73.313524 73.313524 0 0 1 22.918095-97.816381l3.949715-2.438095 31.719619-18.285715c-2.438095-23.161905-2.56-46.665143-0.219429-70.119619l-35.206095-20.333714a73.313524 73.313524 0 0 1-28.891429-96.329143l2.048-3.803428 61.098667-105.813334a73.313524 73.313524 0 0 1 96.329143-28.867047l3.803429 2.048 30.72 17.724952a341.284571 341.284571 0 0 1 64.609523-39.716571l-0.048762-27.89181a73.313524 73.313524 0 0 1 68.754286-73.264762L448.487619 97.52381z m145.798095 283.721142a146.407619 146.407619 0 0 0-200.167619 53.638096 146.773333 146.773333 0 0 0 53.711238 200.362666 146.407619 146.407619 0 0 0 200.167619-53.638095 146.773333 146.773333 0 0 0-53.711238-200.362667z m-136.655238 90.258286a73.118476 73.118476 0 0 1 96.182857-28.842667l3.803429 2.048 3.657143 2.267429a73.508571 73.508571 0 0 1 23.210666 98.011429 73.118476 73.118476 0 0 1-99.961904 26.819047 73.48419 73.48419 0 0 1-26.892191-100.303238z\" p-id=\"9016\" fill=\"#40A9FF\"></path></svg>";
static const char kSvg_ANT_NavAbout[] = "<?xml version=\"1.0\" standalone=\"no\"?><!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\"><svg t=\"1749462999139\" class=\"icon\" viewBox=\"0 0 1024 1024\" version=\"1.1\" xmlns=\"http://www.w3.org/2000/svg\" p-id=\"8017\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\"256\" height=\"256\"><path d=\"M512 97.52381c228.912762 0 414.47619 185.563429 414.47619 414.47619s-185.563429 414.47619-414.47619 414.47619S97.52381 740.912762 97.52381 512 283.087238 97.52381 512 97.52381z m45.32419 560.761904h-68.534857v73.142857h68.534857v-73.142857z m-4.047238-362.008381c-44.27581-8.97219-92.233143-3.120762-126.268952 33.328762C396.629333 362.22781 390.095238 388.047238 390.095238 445.001143h68.266667v-16.579048c0.316952-29.232762 5.241905-59.367619 47.957333-67.779047a68.998095 68.998095 0 0 1 60.367238 15.506285c15.213714 14.652952 15.555048 47.835429-9.020952 64.365715l-38.473143 25.6-4.315429 2.852571c-19.18781 13.336381-26.087619 28.379429-26.087619 49.883429V609.52381h68.534857v-65.731048c0-14.09219 6.729143-27.355429 24.722286-38.863238 17.968762-11.483429 68.120381-34.718476 68.120381-96.134095 0-61.391238-52.638476-103.594667-96.889905-112.518096z\" p-id=\"8018\" fill=\"#bfbfbf\"></path></svg>";
static const char kSvg_ANT_NavAboutActive[] = "<?xml version=\"1.0\" standalone=\"no\"?><!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\"><svg t=\"1749471792004\" class=\"icon\" viewBox=\"0 0 1024 1024\" version=\"1.1\" xmlns=\"http://www.w3.org/2000/svg\" p-id=\"8017\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\"256\" height=\"256\"><path d=\"M512 97.52381c228.912762 0 414.47619 185.563429 414.47619 414.47619s-185.563429 414.47619-414.47619 414.47619S97.52381 740.912762 97.52381 512 283.087238 97.52381 512 97.52381z m45.32419 560.761904h-68.534857v73.142857h68.534857v-73.142857z m-4.047238-362.008381c-44.27581-8.97219-92.233143-3.120762-126.268952 33.328762C396.629333 362.22781 390.095238 388.047238 390.095238 445.001143h68.266667v-16.579048c0.316952-29.232762 5.241905-59.367619 47.957333-67.779047a68.998095 68.998095 0 0 1 60.367238 15.506285c15.213714 14.652952 15.555048 47.835429-9.020952 64.365715l-38.473143 25.6-4.315429 2.852571c-19.18781 13.336381-26.087619 28.379429-26.087619 49.883429V609.52381h68.534857v-65.731048c0-14.09219 6.729143-27.355429 24.722286-38.863238 17.968762-11.483429 68.120381-34.718476 68.120381-96.134095 0-61.391238-52.638476-103.594667-96.889905-112.518096z\" p-id=\"8018\" fill=\"#40A9FF\"></path></svg>";
static const char kSvg_ANT_MsgInfo[] = "<?xml version=\"1.0\" standalone=\"no\"?><!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\"><svg t=\"1748966138445\" class=\"icon\" viewBox=\"0 0 1024 1024\" version=\"1.1\" xmlns=\"http://www.w3.org/2000/svg\" p-id=\"22868\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\"256\" height=\"256\"><path d=\"M512 0C230.4 0 0 230.4 0 512s230.4 512 512 512 512-230.4 512-512-230.4-512-512-512z m64 768c0 38.4-25.6 64-64 64s-64-25.6-64-64V512c0-38.4 25.6-64 64-64s64 25.6 64 64v256z m0-480c0 19.2-12.8 32-32 32h-64c-19.2 0-32-12.8-32-32v-64c0-19.2 12.8-32 32-32h64c19.2 0 32 12.8 32 32v64z\" fill=\"#1296db\" p-id=\"22869\"></path></svg>";
static const char kSvg_ANT_MsgSuccess[] = "<?xml version=\"1.0\" standalone=\"no\"?><!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\"><svg t=\"1748881804271\" class=\"icon\" viewBox=\"0 0 1024 1024\" version=\"1.1\" xmlns=\"http://www.w3.org/2000/svg\" p-id=\"25930\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\"256\" height=\"256\"><path d=\"M983.770989 312.710211a510.243964 510.243964 0 0 0-109.714155-162.742664A510.292726 510.292726 0 0 0 512.000122 0.000488 510.292726 510.292726 0 0 0 149.94341 149.943166 510.14644 510.14644 0 0 0 0.000731 511.999878a510.219583 510.219583 0 0 0 149.942679 362.056712A510.219583 510.219583 0 0 0 512.000122 1023.999269a510.170821 510.170821 0 0 0 362.056712-149.942679A510.41463 510.41463 0 0 0 1023.999512 511.999878a508.82987 508.82987 0 0 0-40.228523-199.289667z m-200.898809 61.976307L472.283598 695.246898a31.622058 31.622058 0 0 1-44.251376 1.267808l-187.952538-172.763223a31.768343 31.768343 0 0 1 21.479593-55.076506c7.972562 0 15.603791 2.974473 21.455213 8.362657l165.205136 151.868772 289.084609-298.37374c6.046469-6.217135 14.140936-9.654846 22.796163-9.654846a31.768343 31.768343 0 0 1 22.771782 53.808698z\" fill=\"#2ABB50\" p-id=\"25931\"></path></svg>";
static const char kSvg_ANT_MsgError[] = "<?xml version=\"1.0\" standalone=\"no\"?><!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\"><svg t=\"1748966098563\" class=\"icon\" viewBox=\"0 0 1024 1024\" version=\"1.1\" xmlns=\"http://www.w3.org/2000/svg\" p-id=\"18302\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\"256\" height=\"256\"><path d=\"M512 0a512 512 0 0 1 512 512 512 512 0 0 1-512 512 512 512 0 0 1-512-512 512 512 0 0 1 512-512z m77.531429 512l129.316571-129.316571a54.857143 54.857143 0 0 0-77.531429-77.531429L512 434.395429 382.683429 305.152a54.857143 54.857143 0 1 0-77.531429 77.531429L434.395429 512 305.152 641.316571a54.857143 54.857143 0 1 0 77.531429 77.531429L512 589.604571l129.316571 129.243429a54.857143 54.857143 0 1 0 77.531429-77.531429L589.604571 512z\" fill=\"#FF5252\" p-id=\"18303\"></path></svg>";
static const char kSvg_ANT_MsgWarning[] = "<?xml version=\"1.0\" standalone=\"no\"?><!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\"><svg t=\"1748966070203\" class=\"icon\" viewBox=\"0 0 1024 1024\" version=\"1.1\" xmlns=\"http://www.w3.org/2000/svg\" p-id=\"14505\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\"256\" height=\"256\"><path d=\"M512 65C265.13 65 65 265.13 65 512s200.13 447 447 447 447-200.13 447-447S758.87 65 512 65z m-40 187a40 40 0 0 1 80 0v336.67a40 40 0 1 1-80 0z m93.87 506.13A53.87 53.87 0 0 1 512 812a53.87 53.87 0 0 1-53.87-53.87A53.87 53.87 0 0 1 512 704.26a53.87 53.87 0 0 1 53.87 53.87z\" fill=\"#F9A825\" p-id=\"14506\"></path></svg>";
static const char kSvg_ANT_AvatarGuest[] = "<?xml version=\"1.0\" standalone=\"no\"?><!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\"><svg t=\"1749057851895\" class=\"icon\" viewBox=\"0 0 1024 1024\" version=\"1.1\" xmlns=\"http://www.w3.org/2000/svg\" p-id=\"10057\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\"256\" height=\"256\"><path d=\"M1023.878734 511.939367A511.939367 511.939367 0 1 1 511.939367 0a511.939367 511.939367 0 0 1 511.939367 511.939367z\" fill=\"#D9D9D9\" p-id=\"10058\"></path><path d=\"M511.939367 255.969684a191.977263 191.977263 0 1 0 191.977263 191.977262 191.977263 191.977263 0 0 0-191.977263-191.977262z m0 479.943156c-108.787116 0-305.243848 43.834808-365.716685 134.384084a511.939367 511.939367 0 0 0 731.433371 0C817.183215 779.747649 620.726483 735.91284 511.939367 735.91284z\" fill=\"#8C8C8C\" p-id=\"10059\"></path></svg>";
static const char kSvg_ANT_BrandGithub[] = "<?xml version=\"1.0\" standalone=\"no\"?><!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\"><svg t=\"1748780969460\" class=\"icon\" viewBox=\"0 0 1028 1024\" version=\"1.1\" xmlns=\"http://www.w3.org/2000/svg\" p-id=\"16679\" width=\"257\" height=\"256\" xmlns:xlink=\"http://www.w3.org/1999/xlink\"><path d=\"M316.806387 841.06986q0-9.197605 9.197605-9.197605t9.197605 9.197605-9.197605 9.197605-9.197605-9.197605zM274.906188 830.850299q0-9.197605 9.197605-9.197605 10.219561 0 10.219561 9.197605 0 10.219561-10.219561 10.219561-9.197605 0-9.197605-10.219561zM509.956088 0q106.283433 0 199.792415 40.367265t163.001996 109.860279 109.860279 163.001996 40.367265 199.792415q0 82.778443-25.037924 158.914172t-69.493014 138.986028-106.794411 110.371257-135.92016 74.091816q-11.241517 2.043912-25.548902 1.021956t-17.373253-18.39521q-2.043912-12.263473-3.576846-32.702595t-2.043912-41.9002-1.021956-40.878244-0.510978-28.61477-3.576846-23.50499-8.175649-27.592814-9.197605-24.015968-6.642715-13.796407q-2.043912-2.043912 14.307385-4.598802t41.389222-9.708583 54.674651-20.439122 55.185629-35.257485 42.411178-54.163673 16.862275-78.179641q0-69.493014-8.175649-108.327345t-17.373253-58.251497q-11.241517-22.483034-25.548902-28.61477 6.131737-33.724551 9.197605-61.317365 2.043912-23.50499 0.510978-44.966068t-12.774451-27.592814q-11.241517-5.10978-33.213573 2.043912t-45.477046 18.39521q-26.570858 13.285429-57.229541 32.702595-15.329341-5.10978-33.724551-9.197605-16.351297-4.087824-38.834331-7.153693t-50.075848-3.065868-52.630739 3.065868-44.45509 6.131737q-22.483034 4.087824-41.9002 9.197605-30.658683-17.373253-57.229541-29.636727-23.50499-11.241517-45.988024-18.39521t-32.702595-3.065868q-11.241517 5.10978-13.796407 26.05988t-1.532934 45.477046q1.021956 28.61477 5.10978 63.361277-14.307385 15.329341-25.548902 36.790419-10.219561 18.39521-17.884232 44.966068t-7.664671 59.273453q0 47.00998 9.708583 82.778443t26.05988 61.828343 37.301397 43.944112 42.411178 29.125749q51.097804 26.570858 116.502994 30.658683-8.175649 9.197605-14.307385 17.373253-5.10978 7.153693-9.197605 13.796407t-4.087824 9.708583-0.510978 7.664671-1.532934 9.708583l-2.043912 10.219561q-10.219561 4.087824-21.461078 7.153693-9.197605 3.065868-20.439122 5.10978t-22.483034 2.043912-27.592814-11.752495-33.213573-27.081836-30.658683-30.147705-19.928144-20.9501q-6.131737-5.10978-16.351297-8.686627t-20.439122-5.620758-18.39521-2.55489-11.241517 0.510978q-6.131737 3.065868-9.197605 8.686627t5.10978 8.686627q4.087824 2.043912 11.752495 7.664671t15.840319 13.285429 15.329341 15.329341 10.219561 12.774451 9.197605 22.483034 19.928144 35.768463 36.790419 32.702595 61.828343 14.307385q26.570858 0 40.367265-0.510978t20.9501-1.532934l0 78.690619q0 6.131737-1.532934 13.796407t-5.620758 13.285429-11.241517 8.175649-18.39521-0.510978q-2.043912-1.021956-6.131737-1.021956-74.602794-26.570858-136.942116-74.091816t-107.816367-110.882236-71.025948-139.497006-25.548902-159.936128q0-106.283433 40.367265-199.792415t109.860279-163.001996 163.512974-109.860279 200.303393-40.367265zM380.167665 878.882236q4.087824 1.021956 6.131737 2.043912 5.10978 3.065868-6.131737 4.087824l0-6.131737zM359.728543 833.916168q0-10.219561 9.197605-10.219561t9.197605 10.219561q0 9.197605-9.197605 9.197605t-9.197605-9.197605zM177.820359 731.720559q0-6.131737 7.153693-6.131737t7.153693 6.131737q0 7.153693-7.153693 7.153693t-7.153693-7.153693zM246.291417 809.389222q0-9.197605 9.197605-9.197605t9.197605 9.197605q0 10.219561-9.197605 10.219561t-9.197605-10.219561zM207.457086 747.0499q7.153693 0 7.153693 7.153693t-7.153693 7.153693-7.153693-7.153693 7.153693-7.153693zM217.676647 780.774451q0-9.197605 9.197605-9.197605t9.197605 9.197605q0 10.219561-9.197605 10.219561t-9.197605-10.219561z\" p-id=\"16680\" fill=\"#bfbfbf\"></path></svg>";
static const char kSvg_ANT_Back[] = "<?xml version=\"1.0\" standalone=\"no\"?><!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\"><svg t=\"1754469003893\" class=\"icon\" viewBox=\"0 0 1024 1024\" version=\"1.1\" xmlns=\"http://www.w3.org/2000/svg\" p-id=\"10041\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\"128\" height=\"128\"><path d=\"M156.608 487.8592c1.156267-1.137067 2.648533-1.6224 3.918933-2.558933l306.666667-301.569067c13.195733-12.970667 34.586667-12.970667 47.781333 0 13.194667 12.978133 13.194667 34.013867 0 46.987733L263.3024 478.210133l579.2032 0c18.978133 0 34.362667 15.128533 34.362667 33.789867 0 18.6624-15.384533 33.793067-34.362667 33.793067L263.3024 545.793067l251.671467 247.486933c13.194667 12.971733 13.194667 34.010667 0 46.984533-13.194667 12.974933-34.586667 12.974933-47.781333 0l-306.666667-301.5616c-1.269333-0.939733-2.762667-1.421867-3.918933-2.562133-6.334933-6.2304-9.240533-14.340267-9.477333-22.5024C147.367467 502.200533 150.273067 494.090667 156.608 487.8592L156.608 487.8592zM156.608 487.8592\" fill=\"#FFFFFF\" p-id=\"10042\"></path></svg>";
static const char kSvg_ANT_TabClose[] = "<?xml version=\"1.0\" standalone=\"no\"?><!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\"><svg t=\"1754131917002\" class=\"icon\" viewBox=\"0 0 1024 1024\" version=\"1.1\" xmlns=\"http://www.w3.org/2000/svg\" p-id=\"4983\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\"256\" height=\"256\"><path d=\"M592.213333 511.715556l317.667556-317.724445c22.186667-22.186667 22.186667-58.368 0-80.497778a57.059556 57.059556 0 0 0-80.440889 0L511.658667 431.217778 193.991111 113.493333a57.116444 57.116444 0 0 0-80.497778 0 57.116444 57.116444 0 0 0 0 80.497778L431.217778 511.715556 113.436444 829.44a57.116444 57.116444 0 0 0 0 80.497778c22.186667 22.129778 58.368 22.129778 80.497778 0l317.724445-317.724445 317.781333 317.724445a57.059556 57.059556 0 0 0 80.440889 0c22.186667-22.186667 22.186667-58.368 0-80.497778L592.213333 511.715556z\" fill=\"#2c2c2c\" p-id=\"4984\"></path></svg>";
static const char kSvg_ANT_Search[] = "<?xml version=\"1.0\" standalone=\"no\"?><!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\"><svg t=\"1753374043305\" class=\"icon\" viewBox=\"0 0 1024 1024\" version=\"1.1\" xmlns=\"http://www.w3.org/2000/svg\" p-id=\"4358\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\"256\" height=\"256\"><path d=\"M1005.312 914.752l-198.528-198.464A448 448 0 1 0 0 448a448 448 0 0 0 716.288 358.784l198.4 198.4a64 64 0 1 0 90.624-90.432zM448 767.936A320 320 0 1 1 448 128a320 320 0 0 1 0 640z\" fill=\"#8a8a8a\" p-id=\"4359\"></path></svg>";
static const char kSvg_ANT_Prev[] = "<?xml version=\"1.0\" standalone=\"no\"?><!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\"><svg t=\"1754220141302\" class=\"icon\" viewBox=\"0 0 1024 1024\" version=\"1.1\" xmlns=\"http://www.w3.org/2000/svg\" p-id=\"3390\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\"256\" height=\"256\"><path d=\"M691.8656 172.544a32 32 0 0 0-45.2608-45.2096l-358.4 358.4a37.12 37.12 0 0 0 0 52.5312l358.4 358.4a32 32 0 0 0 45.2608-45.2608L352.4096 512l339.456-339.456z\" fill=\"#5A5A68\" p-id=\"3391\"></path></svg>";
static const char kSvg_ANT_Next[] = "<?xml version=\"1.0\" standalone=\"no\"?><!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\"><svg t=\"1754220146171\" class=\"icon\" viewBox=\"0 0 1024 1024\" version=\"1.1\" xmlns=\"http://www.w3.org/2000/svg\" p-id=\"3552\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\"256\" height=\"256\"><path d=\"M671.5392 512L332.1344 172.544a32 32 0 0 1 45.2608-45.2096l358.4 358.4a37.12 37.12 0 0 1 0 52.5312l-358.4 358.4a32 32 0 0 1-45.2608-45.2608L671.5904 512z\" fill=\"#5A5A68\" p-id=\"3553\"></path></svg>";
static const char kSvg_ANT_ArrowUp[] = "<?xml version=\"1.0\" standalone=\"no\"?><!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\"><svg t=\"1752495472859\" class=\"icon\" viewBox=\"0 0 1024 1024\" version=\"1.1\" xmlns=\"http://www.w3.org/2000/svg\" p-id=\"16946\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\"256\" height=\"256\"><path d=\"M785.92 677.952a38.464 38.464 0 0 0 7.616-51.072l-4.096-5.184L542.08 349.376a43.392 43.392 0 0 0-57.856-4.352l-4.864 4.48-244.928 272.32a38.528 38.528 0 0 0 3.648 56.192 43.52 43.52 0 0 0 54.208 1.024l4.928-4.48L510.976 435.84l215.872 238.72a43.392 43.392 0 0 0 53.632 7.232l5.44-3.904z\" fill=\"#8a8a8a\" p-id=\"16947\"></path></svg>";
static const char kSvg_ANT_ArrowDown[] = "<?xml version=\"1.0\" standalone=\"no\"?><!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\"><svg t=\"1752495505784\" class=\"icon\" viewBox=\"0 0 1024 1024\" version=\"1.1\" xmlns=\"http://www.w3.org/2000/svg\" p-id=\"18781\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\"256\" height=\"256\"><path d=\"M785.92 346.048a38.464 38.464 0 0 1 7.616 51.072l-4.096 5.184-247.296 272.32a43.392 43.392 0 0 1-57.856 4.352l-4.864-4.48-244.928-272.32a38.528 38.528 0 0 1 3.648-56.192 43.52 43.52 0 0 1 54.208-1.024l4.928 4.48L510.976 588.16l215.872-238.72a43.392 43.392 0 0 1 53.632-7.232l5.44 3.904z\" fill=\"#8a8a8a\" p-id=\"18782\"></path></svg>";
static const char kSvg_ANT_ArrowRight[] = "<?xml version=\"1.0\" standalone=\"no\"?><!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\"><svg t=\"1752638725892\" class=\"icon\" viewBox=\"0 0 1024 1024\" version=\"1.1\" xmlns=\"http://www.w3.org/2000/svg\" p-id=\"5008\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\"256\" height=\"256\"><path d=\"M534.826667 935.466667a47.36 47.36 0 0 1-66.986667-66.773334L835.413333 501.333333 467.84 133.973333a47.36 47.36 0 1 1 66.986667-66.773333l400.64 400.64a47.36 47.36 0 0 1 0 66.986667z\" fill=\"#8a8a8a\" p-id=\"5009\"></path></svg>";
static const char kSvg_ANT_ArrowRightSmall[] = "<?xml version=\"1.0\" standalone=\"no\"?><!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\"><svg t=\"1754288391529\" class=\"icon\" viewBox=\"0 0 1024 1024\" version=\"1.1\" xmlns=\"http://www.w3.org/2000/svg\" p-id=\"6189\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\"256\" height=\"256\"><path d=\"M416.213333 269.952a32 32 0 0 1 45.269334 0l213.333333 213.333333a32 32 0 0 1 0 45.226667l-213.333333 213.333333a32 32 0 0 1-45.226667-45.226666l190.677333-190.72-190.72-190.72a32 32 0 0 1 0-45.226667z\" fill=\"#8a8a8a\" p-id=\"6190\"></path></svg>";
static const char kSvg_ANT_NoData[] = "<?xml version=\"1.0\" standalone=\"no\"?><!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\"><svg t=\"1750058934829\" class=\"icon\" viewBox=\"0 0 1638 1024\" version=\"1.1\" xmlns=\"http://www.w3.org/2000/svg\" p-id=\"16821\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\"409.5\" height=\"256\"><path d=\"M0 844.8c0 97.45 361.097 176.47 806.537 176.47s806.502-79.02 806.502-176.47c0-97.417-361.097-176.401-806.502-176.401C361.097 668.399 0 747.383 0 844.8z\" fill=\"#F5F5F5\" p-id=\"16822\"></path><path d=\"M1398.784 580.164H214.254V330.65l259.62-294.878C486.4 13.107 509.132 0.478 531.796 0.478h549.445c22.698 0 45.397 12.63 60.484 37.82l257.092 292.352v249.514zM239.479 555.008h1134.149V338.261L1121.553 50.86c-12.561-17.613-25.19-27.648-40.311-27.648H531.797c-15.12 0-27.716 10.069-37.82 25.19L239.446 340.753v214.22z\" fill=\"#D9D9D9\" p-id=\"16823\"></path><path d=\"M1048.474 413.833c0-40.312 25.19-73.08 55.466-73.08h282.283v456.192c0 52.907-32.768 98.304-75.64 98.304H302.455c-40.345 0-75.605-42.871-75.605-98.304V340.753h282.283c30.242 0 55.432 32.768 55.432 73.08s25.19 73.079 55.433 73.079H993.04c30.242 2.526 55.433-32.768 55.433-73.08z\" fill=\"#FAFAFA\" p-id=\"16824\"></path><path d=\"M1310.583 907.844H302.455c-47.889 0-88.234-50.415-88.234-110.933V328.192h294.912c37.785 0 68.028 37.786 68.028 85.675 0 32.768 20.138 60.484 42.837 60.484H993.04c22.665 0 42.837-27.716 42.837-60.484 0-47.89 30.243-85.675 68.062-85.675h294.878v468.787c0 60.484-37.82 110.9-88.235 110.9zM239.445 353.348v443.597c0 47.89 27.717 85.675 63.01 85.675h1008.128c35.294 0 63.01-37.786 63.01-85.675V353.348H1103.94c-22.698 0-42.871 27.717-42.871 60.485 0 47.889-30.242 85.674-68.028 85.674H619.998c-37.786 0-68.028-37.785-68.028-85.674 0-32.768-20.139-63.01-42.837-63.01H239.445v2.525z\" fill=\"#D9D9D9\" p-id=\"16825\"></path></svg>";
static const char kSvg_ANT_GoTo[] = "<?xml version=\"1.0\" standalone=\"no\"?><!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\"><svg t=\"1755075479689\" class=\"icon\" viewBox=\"0 0 1024 1024\" version=\"1.1\" xmlns=\"http://www.w3.org/2000/svg\" p-id=\"1491\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\"256\" height=\"256\"><path d=\"M989.25568 537.0368c-6.67904-19.55328-27.89632-29.952-47.47264-23.2704a37.2352 37.2352 0 0 0-23.20896 23.2704h-2.58048v328.71424c-1.40032 27.31264-23.20384 49.11616-50.51648 50.5856H157.9776c-27.31008-1.46944-49.1136-23.27296-50.60864-50.5856V158.05696c1.46688-27.3408 23.26784-49.14688 50.60864-50.61376h303.1936c52.19584 0 51.84768-75.78112 0-75.78112H161.87904c-73.82016-0.06144-130.29376 55.62112-130.29376 128.29696v703.62624c0 72.73216 56.4736 128.23552 130.29376 128.23552h713.14688c73.90464 0 116.81024-53.62688 116.81024-126.0672V537.0368h-2.58048z\" fill=\"#2c2c2c\" p-id=\"1492\"></path><path d=\"M875.02848 1007.1808H161.87904c-39.76704 0-76.56192-14.65088-103.6032-41.25184-27.11808-26.67776-42.05312-63.02464-42.05312-102.34368V159.95904c0-39.28064 14.91712-75.6224 42.00192-102.32576C85.2608 30.98368 122.02752 16.30464 161.75872 16.30464h299.41248c35.6992 0 54.37696 26.83648 54.37696 53.34528 0 13.76512-4.89216 26.68288-13.77792 36.37248-7.02208 7.65696-19.76576 16.78336-40.59904 16.78336H158.42304a38.1312 38.1312 0 0 0-35.69152 35.69664v706.79552a38.15936 38.15936 0 0 0 35.69408 35.68128h706.60352a37.98528 37.98528 0 0 0 35.60448-35.65056V521.6768h8.27392c6.35136-10.39872 16.2432-18.46528 27.91424-22.44864a52.7104 52.7104 0 0 1 62.10304 22.44864h8.27136v344.07424c0.00256 84.5952-53.11488 141.42976-132.16768 141.42976zM161.87904 47.02464c-65.57696 0-114.93376 48.55296-114.93376 112.93696v703.62624c0 64.35072 49.41056 112.87552 114.93376 112.87552h713.14688c61.62688 0 101.45024-43.456 101.45024-110.7072V547.14624l-1.75872-5.1456a21.99808 21.99808 0 0 0-27.97568-13.69856 21.94944 21.94944 0 0 0-13.62176 13.6576l-1.76896 5.23264-0.02048 319.3472c-1.8048 35.23584-29.76512 63.23712-65.03168 65.13408l-0.82432 0.02304H157.56544l-0.41216-0.02304c-35.2-1.8944-63.1936-29.8752-65.12128-65.08288l-0.02304-0.83968V157.64736l0.02304-0.41216a68.79232 68.79232 0 0 1 65.12128-65.1264l0.82176-0.02304h303.1936c7.76192 0 13.80352-2.29632 17.9584-6.82752 3.6224-3.94752 5.69856-9.6384 5.69856-15.60832 0-10.43712-6.1952-22.62528-23.65696-22.62528H161.87904z\" fill=\"#2c2c2c\" p-id=\"1493\"></path><path d=\"M667.71968 109.20192h189.07648l-359.1936 360.92416c-15.15776 15.18592-15.15776 39.73888 0 54.9248 14.94272 15.05792 39.3216 15.15264 54.41792 0.20224a1.0752 1.0752 0 0 1 0.17152-0.20224L914.11712 161.36448v194.9696c0 21.3376 17.34912 38.656 38.68416 38.656 21.32992 0 38.68416-17.3184 38.68416-38.656V71.69024a38.76864 38.76864 0 0 0-12.84352-28.4544 38.4512 38.4512 0 0 0-27.4816-11.6352h-283.4432c-21.4528 0.84736-38.10304 18.90304-37.28128 40.32256 0.83456 20.25472 17.06752 36.48768 37.28384 37.27872z\" fill=\"#2c2c2c\" p-id=\"1494\"></path><path d=\"M524.90496 551.76448c-0.00256 0 0 0 0 0-14.464-0.00256-28.032-5.64736-38.20288-15.89504-21.056-21.09184-21.05856-55.46752 0.03072-76.5952L819.84 124.56192l-152.71936-0.01024c-28.02176-1.09824-50.87488-23.936-52.02944-51.9936-1.14176-29.88544 22.1952-55.12448 52.02176-56.30208l0.60672-0.0128h283.4432c14.28224 0.04096 28.13184 5.83424 38.1312 15.91552a54.30528 54.30528 0 0 1 17.55392 39.36v284.81536c0 29.78304-24.24576 54.016-54.04416 54.016s-54.04416-24.2304-54.04416-54.016V198.57408L564.06016 534.90176l-0.08704 0.13312-1.14176 1.12896a53.56032 53.56032 0 0 1-37.9264 15.60064z m143.1552-457.92256h225.69472L508.4928 480.96c-9.15968 9.17504-9.15968 24.07936-0.01792 33.23648a22.95808 22.95808 0 0 0 16.43008 6.84544 23.02464 23.02464 0 0 0 15.92832-6.33856c0.17664-0.192 0.36096-0.37888 0.54784-0.5632l388.096-389.98528v232.1792c0 12.84608 10.46272 23.296 23.32416 23.296s23.32416-10.44992 23.32416-23.296V71.69024a23.41888 23.41888 0 0 0-7.744-17.024l-0.75264-0.72192a23.22944 23.22944 0 0 0-16.50944-6.98112h-283.0592c-12.79488 0.65024-22.76608 11.52768-22.272 24.37632 0.49408 12.04224 10.25536 21.89568 22.272 22.5024z\" fill=\"#2c2c2c\" p-id=\"1495\"></path></svg>";
static const char kSvg_ANT_Eye[] = "<?xml version=\"1.0\" standalone=\"no\"?><!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\"><svg t=\"1749204614107\" class=\"icon\" viewBox=\"0 0 1024 1024\" version=\"1.1\" xmlns=\"http://www.w3.org/2000/svg\" p-id=\"2672\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\"256\" height=\"256\"><path d=\"M512 74q-226 0-406 282-8 13-8 28t9 27q179 283 405 283t406-283q8-12 8-27t-8-28Q738 74 512 74z m0 124q51 0 93.5 25t67.5 67.5q25 42.5 25 93T673 477q-25 43-67.5 68T512 570q-51 0-93.5-25T351 477q-25-43-25-93.5t25-93q25-42.5 67.5-67.5t93.5-25z m0 310q-34 0-62.5-16.5t-45-45Q388 418 388 384t16.5-62.5q16.5-28.5 45-45T512 260q34 0 62.5 16.5t45 45Q636 350 636 384t-16.5 62.5q-16.5 28.5-45 45T512 508z\" fill=\"#dbdbdb\" p-id=\"2673\"></path></svg>";
static const char kSvg_ANT_EyeSlash[] = "<?xml version=\"1.0\" standalone=\"no\"?><!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\"><svg t=\"1749204633282\" class=\"icon\" viewBox=\"0 0 1024 1024\" version=\"1.1\" xmlns=\"http://www.w3.org/2000/svg\" p-id=\"3704\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\"256\" height=\"256\"><path d=\"M907 391q12 23 0 47-131 276-395 276-99 0-180-39l112-112q30 13 64 13 44 0 81-22t59-59q22-37 22-81 0-34-13-64l130-130q70 64 120 170l1 1h-1zM849 85q2 2 2 5.5t-2 5.5L194 751q-2 2-5 2t-6-2l-39-39q-2-2-2-5.5t2-5.5l93-92q-70-65-120-171-11-24 0-47 131-277 395-277 99 0 180 40L799 46q2-2 5.5-2t5.5 2l39 39zM578 268q-33-16-70-16-44 0-81 22t-58.5 59Q347 370 347 414q0 37 15 69l216-215z\" fill=\"#dbdbdb\" p-id=\"3705\"></path></svg>";
static const char kSvg_ANT_CarouselArrow[] = "<?xml version=\"1.0\" standalone=\"no\"?><!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\"><svg t=\"1753188757474\" class=\"icon\" viewBox=\"0 0 1024 1024\" version=\"1.1\" xmlns=\"http://www.w3.org/2000/svg\" p-id=\"18752\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\"256\" height=\"256\"><path d=\"M512 0a512 512 0 1 0 0 1024 512 512 0 0 0 0-1024z m0 960.030118a448.030118 448.030118 0 1 1 0-896.060236 448.030118 448.030118 0 0 1 0 896.060236z m145.167059-476.16L412.069647 238.832941a31.984941 31.984941 0 0 0-45.236706 45.296941L594.763294 512 366.832941 739.870118a31.984941 31.984941 0 1 0 45.176471 45.296941l245.037176-245.037177a31.382588 31.382588 0 0 0 8.312471-28.129882 31.322353 31.322353 0 0 0-8.312471-28.129882z\" fill=\"#ffffff\" p-id=\"18753\"></path></svg>";
#endif

static const IconAsset kIcons[] = {
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Close, "close", "icons/close.svg", kSvg_Close, 313},
#else
    {Icon::Close, "close", "icons/close.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Minimize, "minimize", "icons/minimize.svg", kSvg_Minimize, 331},
#else
    {Icon::Minimize, "minimize", "icons/minimize.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Maximize, "maximize", "icons/maximize.svg", kSvg_Maximize, 331},
#else
    {Icon::Maximize, "maximize", "icons/maximize.svg", nullptr, 0},
#endif
    {Icon::Restore, "restore", "icons/restore.svg", nullptr, 0},
    {Icon::Help, "help", "icons/help.svg", nullptr, 0},
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Info, "info", "icons/info.svg", kSvg_Info, 347},
#else
    {Icon::Info, "info", "icons/info.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Warning, "warning", "icons/warning.svg", kSvg_Warning, 366},
#else
    {Icon::Warning, "warning", "icons/warning.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Error, "error", "icons/error.svg", kSvg_Error, 253},
#else
    {Icon::Error, "error", "icons/error.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Settings, "settings", "icons/settings.svg", kSvg_Settings, 1011},
#else
    {Icon::Settings, "settings", "icons/settings.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Menu, "menu", "icons/menu.svg", kSvg_Menu, 346},
#else
    {Icon::Menu, "menu", "icons/menu.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::File, "file", "icons/file.svg", kSvg_File, 337},
#else
    {Icon::File, "file", "icons/file.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Folder, "folder", "icons/folder.svg", kSvg_Folder, 311},
#else
    {Icon::Folder, "folder", "icons/folder.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::FolderOpen, "folder_open", "icons/folder_open.svg", kSvg_FolderOpen, 361},
#else
    {Icon::FolderOpen, "folder_open", "icons/folder_open.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Image, "image", "icons/image.svg", kSvg_Image, 369},
#else
    {Icon::Image, "image", "icons/image.svg", nullptr, 0},
#endif
    {Icon::Audio, "audio", "icons/audio.svg", nullptr, 0},
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Video, "video", "icons/video.svg", kSvg_Video, 329},
#else
    {Icon::Video, "video", "icons/video.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Archive, "archive", "icons/archive.svg", kSvg_Archive, 361},
#else
    {Icon::Archive, "archive", "icons/archive.svg", nullptr, 0},
#endif
    {Icon::Pdf, "pdf", "icons/pdf.svg", nullptr, 0},
    {Icon::Word, "word", "icons/word.svg", nullptr, 0},
    {Icon::Excel, "excel", "icons/excel.svg", nullptr, 0},
    {Icon::Ppt, "ppt", "icons/ppt.svg", nullptr, 0},
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Code, "code", "icons/code.svg", kSvg_Code, 307},
#else
    {Icon::Code, "code", "icons/code.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::New, "new", "icons/new.svg", kSvg_New, 304},
#else
    {Icon::New, "new", "icons/new.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Open, "open", "icons/open.svg", kSvg_Open, 311},
#else
    {Icon::Open, "open", "icons/open.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Save, "save", "icons/save.svg", kSvg_Save, 392},
#else
    {Icon::Save, "save", "icons/save.svg", nullptr, 0},
#endif
    {Icon::SaveAs, "save_as", "icons/save_as.svg", nullptr, 0},
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Delete, "delete", "icons/delete.svg", kSvg_Delete, 374},
#else
    {Icon::Delete, "delete", "icons/delete.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Edit, "edit", "icons/edit.svg", kSvg_Edit, 365},
#else
    {Icon::Edit, "edit", "icons/edit.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Copy, "copy", "icons/copy.svg", kSvg_Copy, 351},
#else
    {Icon::Copy, "copy", "icons/copy.svg", nullptr, 0},
#endif
    {Icon::Paste, "paste", "icons/paste.svg", nullptr, 0},
    {Icon::Cut, "cut", "icons/cut.svg", nullptr, 0},
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Undo, "undo", "icons/undo.svg", kSvg_Undo, 386},
#else
    {Icon::Undo, "undo", "icons/undo.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Redo, "redo", "icons/redo.svg", kSvg_Redo, 380},
#else
    {Icon::Redo, "redo", "icons/redo.svg", nullptr, 0},
#endif
    {Icon::SelectAll, "select_all", "icons/select_all.svg", nullptr, 0},
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Find, "find", "icons/find.svg", kSvg_Find, 308},
#else
    {Icon::Find, "find", "icons/find.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Replace, "replace", "icons/replace.svg", kSvg_Replace, 400},
#else
    {Icon::Replace, "replace", "icons/replace.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Back, "back", "icons/back.svg", kSvg_Back, 312},
#else
    {Icon::Back, "back", "icons/back.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Forward, "forward", "icons/forward.svg", kSvg_Forward, 314},
#else
    {Icon::Forward, "forward", "icons/forward.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Up, "up", "icons/up.svg", kSvg_Up, 310},
#else
    {Icon::Up, "up", "icons/up.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Refresh, "refresh", "icons/refresh.svg", kSvg_Refresh, 400},
#else
    {Icon::Refresh, "refresh", "icons/refresh.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Home, "home", "icons/home.svg", kSvg_Home, 332},
#else
    {Icon::Home, "home", "icons/home.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::ZoomIn, "zoom_in", "icons/zoom_in.svg", kSvg_ZoomIn, 397},
#else
    {Icon::ZoomIn, "zoom_in", "icons/zoom_in.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::ZoomOut, "zoom_out", "icons/zoom_out.svg", kSvg_ZoomOut, 354},
#else
    {Icon::ZoomOut, "zoom_out", "icons/zoom_out.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Fullscreen, "fullscreen", "icons/fullscreen.svg", kSvg_Fullscreen, 331},
#else
    {Icon::Fullscreen, "fullscreen", "icons/fullscreen.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::FullscreenExit, "fullscreen_exit", "icons/fullscreen_exit.svg", kSvg_FullscreenExit, 331},
#else
    {Icon::FullscreenExit, "fullscreen_exit", "icons/fullscreen_exit.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Grid, "grid", "icons/grid.svg", kSvg_Grid, 404},
#else
    {Icon::Grid, "grid", "icons/grid.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Search, "search", "icons/search.svg", kSvg_Search, 308},
#else
    {Icon::Search, "search", "icons/search.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Filter, "filter", "icons/filter.svg", kSvg_Filter, 290},
#else
    {Icon::Filter, "filter", "icons/filter.svg", nullptr, 0},
#endif
    {Icon::Sort, "sort", "icons/sort.svg", nullptr, 0},
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::SortAsc, "sort_asc", "icons/sort_asc.svg", kSvg_SortAsc, 310},
#else
    {Icon::SortAsc, "sort_asc", "icons/sort_asc.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::SortDesc, "sort_desc", "icons/sort_desc.svg", kSvg_SortDesc, 313},
#else
    {Icon::SortDesc, "sort_desc", "icons/sort_desc.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Check, "check", "icons/check.svg", kSvg_Check, 262},
#else
    {Icon::Check, "check", "icons/check.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Circle, "circle", "icons/circle.svg", kSvg_Circle, 258},
#else
    {Icon::Circle, "circle", "icons/circle.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Square, "square", "icons/square.svg", kSvg_Square, 280},
#else
    {Icon::Square, "square", "icons/square.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Star, "star", "icons/star.svg", kSvg_Star, 339},
#else
    {Icon::Star, "star", "icons/star.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Heart, "heart", "icons/heart.svg", kSvg_Heart, 371},
#else
    {Icon::Heart, "heart", "icons/heart.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Flag, "flag", "icons/flag.svg", kSvg_Flag, 334},
#else
    {Icon::Flag, "flag", "icons/flag.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Tag, "tag", "icons/tag.svg", kSvg_Tag, 355},
#else
    {Icon::Tag, "tag", "icons/tag.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Clock, "clock", "icons/clock.svg", kSvg_Clock, 304},
#else
    {Icon::Clock, "clock", "icons/clock.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Calendar, "calendar", "icons/calendar.svg", kSvg_Calendar, 410},
#else
    {Icon::Calendar, "calendar", "icons/calendar.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Link, "link", "icons/link.svg", kSvg_Link, 371},
#else
    {Icon::Link, "link", "icons/link.svg", nullptr, 0},
#endif
    {Icon::Unlink, "unlink", "icons/unlink.svg", nullptr, 0},
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Web, "web", "icons/web.svg", kSvg_Web, 409},
#else
    {Icon::Web, "web", "icons/web.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Download, "download", "icons/download.svg", kSvg_Download, 370},
#else
    {Icon::Download, "download", "icons/download.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Upload, "upload", "icons/upload.svg", kSvg_Upload, 365},
#else
    {Icon::Upload, "upload", "icons/upload.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Cloud, "cloud", "icons/cloud.svg", kSvg_Cloud, 280},
#else
    {Icon::Cloud, "cloud", "icons/cloud.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Share, "share", "icons/share.svg", kSvg_Share, 364},
#else
    {Icon::Share, "share", "icons/share.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::External, "external", "icons/external.svg", kSvg_External, 388},
#else
    {Icon::External, "external", "icons/external.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Play, "play", "icons/play.svg", kSvg_Play, 263},
#else
    {Icon::Play, "play", "icons/play.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Pause, "pause", "icons/pause.svg", kSvg_Pause, 312},
#else
    {Icon::Pause, "pause", "icons/pause.svg", nullptr, 0},
#endif
    {Icon::Stop, "stop", "icons/stop.svg", nullptr, 0},
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Prev, "prev", "icons/prev.svg", kSvg_Prev, 313},
#else
    {Icon::Prev, "prev", "icons/prev.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Next, "next", "icons/next.svg", kSvg_Next, 315},
#else
    {Icon::Next, "next", "icons/next.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Volume, "volume", "icons/volume.svg", kSvg_Volume, 280},
#else
    {Icon::Volume, "volume", "icons/volume.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::VolumeMute, "volume_mute", "icons/volume_mute.svg", kSvg_VolumeMute, 370},
#else
    {Icon::VolumeMute, "volume_mute", "icons/volume_mute.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Mic, "mic", "icons/mic.svg", kSvg_Mic, 418},
#else
    {Icon::Mic, "mic", "icons/mic.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Computer, "computer", "icons/computer.svg", kSvg_Computer, 370},
#else
    {Icon::Computer, "computer", "icons/computer.svg", nullptr, 0},
#endif
    {Icon::ControlPanel, "control_panel", "icons/control_panel.svg", nullptr, 0},
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Cmd, "cmd", "icons/cmd.svg", kSvg_Cmd, 310},
#else
    {Icon::Cmd, "cmd", "icons/cmd.svg", nullptr, 0},
#endif
    {Icon::Registry, "registry", "icons/registry.svg", nullptr, 0},
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Shutdown, "shutdown", "icons/shutdown.svg", kSvg_Shutdown, 308},
#else
    {Icon::Shutdown, "shutdown", "icons/shutdown.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Restart, "restart", "icons/restart.svg", kSvg_Restart, 321},
#else
    {Icon::Restart, "restart", "icons/restart.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Lock, "lock", "icons/lock.svg", kSvg_Lock, 321},
#else
    {Icon::Lock, "lock", "icons/lock.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Unlock, "unlock", "icons/unlock.svg", kSvg_Unlock, 322},
#else
    {Icon::Unlock, "unlock", "icons/unlock.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Key, "key", "icons/key.svg", kSvg_Key, 352},
#else
    {Icon::Key, "key", "icons/key.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Printer, "printer", "icons/printer.svg", kSvg_Printer, 407},
#else
    {Icon::Printer, "printer", "icons/printer.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Battery, "battery", "icons/battery.svg", kSvg_Battery, 326},
#else
    {Icon::Battery, "battery", "icons/battery.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Wifi, "wifi", "icons/wifi.svg", kSvg_Wifi, 401},
#else
    {Icon::Wifi, "wifi", "icons/wifi.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::App, "app", "icons/app.svg", kSvg_App, 462},
#else
    {Icon::App, "app", "icons/app.svg", nullptr, 0},
#endif
    {Icon::Group, "group", "icons/group.svg", nullptr, 0},
    {Icon::Tab, "tab", "icons/tab.svg", nullptr, 0},
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Pin, "pin", "icons/pin.svg", kSvg_Pin, 480},
#else
    {Icon::Pin, "pin", "icons/pin.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Unpin, "unpin", "icons/unpin.svg", kSvg_Unpin, 204},
#else
    {Icon::Unpin, "unpin", "icons/unpin.svg", nullptr, 0},
#endif
    {Icon::Drag, "drag", "icons/drag.svg", nullptr, 0},
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Plus, "plus", "icons/plus.svg", kSvg_Plus, 304},
#else
    {Icon::Plus, "plus", "icons/plus.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Minus, "minus", "icons/minus.svg", kSvg_Minus, 261},
#else
    {Icon::Minus, "minus", "icons/minus.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::History, "history", "icons/history.svg", kSvg_History, 304},
#else
    {Icon::History, "history", "icons/history.svg", nullptr, 0},
#endif
    {Icon::Freq, "freq", "icons/freq.svg", nullptr, 0},
    {Icon::SearchHistory, "search_history", "icons/search_history.svg", nullptr, 0},
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::Clear, "clear", "icons/clear.svg", kSvg_Clear, 346},
#else
    {Icon::Clear, "clear", "icons/clear.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::ANT_ThemeSun, "ant_theme_sun", "icons/ant_theme_sun.svg", kSvg_ANT_ThemeSun, 1983},
#else
    {Icon::ANT_ThemeSun, "ant_theme_sun", "icons/ant_theme_sun.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::ANT_ThemeMoon, "ant_theme_moon", "icons/ant_theme_moon.svg", kSvg_ANT_ThemeMoon, 1184},
#else
    {Icon::ANT_ThemeMoon, "ant_theme_moon", "icons/ant_theme_moon.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::ANT_WindowMin, "ant_window_min", "icons/ant_window_min.svg", kSvg_ANT_WindowMin, 466},
#else
    {Icon::ANT_WindowMin, "ant_window_min", "icons/ant_window_min.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::ANT_WindowMax, "ant_window_max", "icons/ant_window_max.svg", kSvg_ANT_WindowMax, 641},
#else
    {Icon::ANT_WindowMax, "ant_window_max", "icons/ant_window_max.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::ANT_WindowClose, "ant_window_close", "icons/ant_window_close.svg", kSvg_ANT_WindowClose, 693},
#else
    {Icon::ANT_WindowClose, "ant_window_close", "icons/ant_window_close.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::ANT_WindowRestore, "ant_window_restore", "icons/ant_window_restore.svg", kSvg_ANT_WindowRestore, 838},
#else
    {Icon::ANT_WindowRestore, "ant_window_restore", "icons/ant_window_restore.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::ANT_NavHome, "ant_nav_home", "icons/ant_nav_home.svg", kSvg_ANT_NavHome, 770},
#else
    {Icon::ANT_NavHome, "ant_nav_home", "icons/ant_nav_home.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::ANT_NavHomeActive, "ant_nav_home_active", "icons/ant_nav_home_active.svg", kSvg_ANT_NavHomeActive, 770},
#else
    {Icon::ANT_NavHomeActive, "ant_nav_home_active", "icons/ant_nav_home_active.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::ANT_NavFunction, "ant_nav_function", "icons/ant_nav_function.svg", kSvg_ANT_NavFunction, 1765},
#else
    {Icon::ANT_NavFunction, "ant_nav_function", "icons/ant_nav_function.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::ANT_NavFunctionActive, "ant_nav_function_active", "icons/ant_nav_function_active.svg", kSvg_ANT_NavFunctionActive, 1765},
#else
    {Icon::ANT_NavFunctionActive, "ant_nav_function_active", "icons/ant_nav_function_active.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::ANT_NavSettings, "ant_nav_settings", "icons/ant_nav_settings.svg", kSvg_ANT_NavSettings, 2195},
#else
    {Icon::ANT_NavSettings, "ant_nav_settings", "icons/ant_nav_settings.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::ANT_NavSettingsActive, "ant_nav_settings_active", "icons/ant_nav_settings_active.svg", kSvg_ANT_NavSettingsActive, 2195},
#else
    {Icon::ANT_NavSettingsActive, "ant_nav_settings_active", "icons/ant_nav_settings_active.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::ANT_NavAbout, "ant_nav_about", "icons/ant_nav_about.svg", kSvg_ANT_NavAbout, 1234},
#else
    {Icon::ANT_NavAbout, "ant_nav_about", "icons/ant_nav_about.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::ANT_NavAboutActive, "ant_nav_about_active", "icons/ant_nav_about_active.svg", kSvg_ANT_NavAboutActive, 1234},
#else
    {Icon::ANT_NavAboutActive, "ant_nav_about_active", "icons/ant_nav_about_active.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::ANT_MsgInfo, "ant_msg_info", "icons/ant_msg_info.svg", kSvg_ANT_MsgInfo, 655},
#else
    {Icon::ANT_MsgInfo, "ant_msg_info", "icons/ant_msg_info.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::ANT_MsgSuccess, "ant_msg_success", "icons/ant_msg_success.svg", kSvg_ANT_MsgSuccess, 1194},
#else
    {Icon::ANT_MsgSuccess, "ant_msg_success", "icons/ant_msg_success.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::ANT_MsgError, "ant_msg_error", "icons/ant_msg_error.svg", kSvg_ANT_MsgError, 814},
#else
    {Icon::ANT_MsgError, "ant_msg_error", "icons/ant_msg_error.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::ANT_MsgWarning, "ant_msg_warning", "icons/ant_msg_warning.svg", kSvg_ANT_MsgWarning, 648},
#else
    {Icon::ANT_MsgWarning, "ant_msg_warning", "icons/ant_msg_warning.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::ANT_AvatarGuest, "ant_avatar_guest", "icons/ant_avatar_guest.svg", kSvg_ANT_AvatarGuest, 838},
#else
    {Icon::ANT_AvatarGuest, "ant_avatar_guest", "icons/ant_avatar_guest.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::ANT_BrandGithub, "ant_brand_github", "icons/ant_brand_github.svg", kSvg_ANT_BrandGithub, 3805},
#else
    {Icon::ANT_BrandGithub, "ant_brand_github", "icons/ant_brand_github.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::ANT_Back, "ant_back", "icons/ant_back.svg", kSvg_ANT_Back, 1075},
#else
    {Icon::ANT_Back, "ant_back", "icons/ant_back.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::ANT_TabClose, "ant_tab_close", "icons/ant_tab_close.svg", kSvg_ANT_TabClose, 907},
#else
    {Icon::ANT_TabClose, "ant_tab_close", "icons/ant_tab_close.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::ANT_Search, "ant_search", "icons/ant_search.svg", kSvg_ANT_Search, 553},
#else
    {Icon::ANT_Search, "ant_search", "icons/ant_search.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::ANT_Prev, "ant_prev", "icons/ant_prev.svg", kSvg_ANT_Prev, 531},
#else
    {Icon::ANT_Prev, "ant_prev", "icons/ant_prev.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::ANT_Next, "ant_next", "icons/ant_next.svg", kSvg_ANT_Next, 528},
#else
    {Icon::ANT_Next, "ant_next", "icons/ant_next.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::ANT_ArrowUp, "ant_arrow_up", "icons/ant_arrow_up.svg", kSvg_ANT_ArrowUp, 663},
#else
    {Icon::ANT_ArrowUp, "ant_arrow_up", "icons/ant_arrow_up.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::ANT_ArrowDown, "ant_arrow_down", "icons/ant_arrow_down.svg", kSvg_ANT_ArrowDown, 663},
#else
    {Icon::ANT_ArrowDown, "ant_arrow_down", "icons/ant_arrow_down.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::ANT_ArrowRight, "ant_arrow_right", "icons/ant_arrow_right.svg", kSvg_ANT_ArrowRight, 558},
#else
    {Icon::ANT_ArrowRight, "ant_arrow_right", "icons/ant_arrow_right.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::ANT_ArrowRightSmall, "ant_arrow_right_small", "icons/ant_arrow_right_small.svg", kSvg_ANT_ArrowRightSmall, 576},
#else
    {Icon::ANT_ArrowRightSmall, "ant_arrow_right_small", "icons/ant_arrow_right_small.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::ANT_NoData, "ant_no_data", "icons/ant_no_data.svg", kSvg_ANT_NoData, 1832},
#else
    {Icon::ANT_NoData, "ant_no_data", "icons/ant_no_data.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::ANT_GoTo, "ant_go_to", "icons/ant_go_to.svg", kSvg_ANT_GoTo, 4164},
#else
    {Icon::ANT_GoTo, "ant_go_to", "icons/ant_go_to.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::ANT_Eye, "ant_eye", "icons/ant_eye.svg", kSvg_ANT_Eye, 766},
#else
    {Icon::ANT_Eye, "ant_eye", "icons/ant_eye.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::ANT_EyeSlash, "ant_eye_slash", "icons/ant_eye_slash.svg", kSvg_ANT_EyeSlash, 749},
#else
    {Icon::ANT_EyeSlash, "ant_eye_slash", "icons/ant_eye_slash.svg", nullptr, 0},
#endif
#if LIBICON_CORE_ENABLE_EMBED
    {Icon::ANT_CarouselArrow, "ant_carousel_arrow", "icons/ant_carousel_arrow.svg", kSvg_ANT_CarouselArrow, 806},
#else
    {Icon::ANT_CarouselArrow, "ant_carousel_arrow", "icons/ant_carousel_arrow.svg", nullptr, 0},
#endif
};

std::size_t GetIconCount() {
    return sizeof(kIcons) / sizeof(kIcons[0]);
}

const IconAsset* GetAllIcons() {
    return kIcons;
}

const IconAsset* FindIcon(Icon icon) {
    if (icon == Icon::None) {
        return nullptr;
    }
    for (const auto& item : kIcons) {
        if (item.icon == icon) {
            return &item;
        }
    }
    return nullptr;
}

const IconAsset* FindIcon(const char* id) {
    if (id == nullptr) {
        return nullptr;
    }
    for (const auto& item : kIcons) {
        if (std::strcmp(item.id, id) == 0) {
            return &item;
        }
    }
    return nullptr;
}

const char* GetDynamicPath(Icon icon) {
    const auto* item = FindIcon(icon);
    if (item == nullptr) {
        return nullptr;
    }
    return item->fileName;
}

const char* GetDynamicPath(const char* id) {
    const auto* item = FindIcon(id);
    if (item == nullptr) {
        return nullptr;
    }
    return item->fileName;
}

const char* GetEmbeddedSvg(Icon icon) {
    const auto* item = FindIcon(icon);
    if (item == nullptr) {
        return nullptr;
    }
    return item->svg;
}

const char* GetEmbeddedSvg(const char* id) {
    const auto* item = FindIcon(id);
    if (item == nullptr) {
        return nullptr;
    }
    return item->svg;
}

} // namespace icon
