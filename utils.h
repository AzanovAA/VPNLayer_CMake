#pragma once

#define SAFE_DELETE(x) if (x) { delete x; x = NULL; }

enum PROTOCOL_TYPE {  PPTP, L2TP, OpenVPN };


