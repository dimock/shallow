/*************************************************************
  Figure.cpp - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/

#include <Figure.h>
#include <Evaluator.h>
#include <globals.h>

namespace NEngine
{

extern const uint8 Figure::mirrorIndex_[64] =
{
  56, 57, 58, 59, 60, 61, 62, 63,
  48, 49, 50, 51, 52, 53, 54, 55,
  40, 41, 42, 43, 44, 45, 46, 47,
  32, 33, 34, 35, 36, 37, 38, 39,
  24, 25, 26, 27, 28, 29, 30, 31,
  16, 17, 18, 19, 20, 21, 22, 23,
   8,  9, 10, 11, 12, 13, 14, 15,
   0,  1,  2,  3,  4,  5,  6,  7
};

extern const BitMask Figure::pawnCutoffMasks_[2] = { 0xfefefefefefefefe /* left */, 0x7f7f7f7f7f7f7f7f /* right */ };

extern const BitMask Figure::quaterBoard_[2][2] = { { 0xf0f0f0f000000000, 0x0f0f0f0f00000000 },
                                                    { 0x00000000f0f0f0f0, 0x000000000f0f0f0f } };

Figure::Type Figure::toFtype(char c)
{
  if ( 'P' == c )
    return Figure::TypePawn;
  else if ( 'N' == c )
    return Figure::TypeKnight;
  else if ( 'B' == c )
    return Figure::TypeBishop;
  else if ( 'R' == c )
    return Figure::TypeRook;
  else if ( 'Q' == c )
    return Figure::TypeQueen;
  else if ( 'K' == c )
    return Figure::TypeKing;
  return Figure::TypeNone;
}

ScoreType Figure::positionEvaluation(int stage, Figure::Color color, Figure::Type type, int pos)
{
  X_ASSERT(stage > 1 || color > 1 || type > 7 || pos < 0 || pos > 63, "invalid figure params");
  if(color)
    return +EvalCoefficients::positionEvaluations_[stage][type][mirrorIndex_[pos]];
  else
    return -EvalCoefficients::positionEvaluations_[stage][type][pos];
}

char Figure::fromFtype(Figure::Type t)
{
  switch ( t )
  {
  case Figure::TypePawn:
    return 'P';

  case Figure::TypeKnight:
    return 'N';

  case Figure::TypeBishop:
    return 'B';

  case Figure::TypeRook:
    return 'R';

  case Figure::TypeQueen:
    return 'Q';

  case Figure::TypeKing:
    return 'K';
  }

  return ' ';
}

const char * Figure::name(Type type)
{
	switch ( type )
	{
	case Figure::TypePawn:
		return "pawn";

	case Figure::TypeBishop:
		return "bishop";

	case Figure::TypeKnight:
		return "knight";

	case Figure::TypeRook:
		return "rook";

	case Figure::TypeQueen:
		return "queen";

	case Figure::TypeKing:
		return "king";
	}
	return "none";
}

//////////////////////////////////////////////////////////////////////////
uint8 FiguresCounter::s_transposeIndex_[] =
{
  0,  8, 16, 24, 32, 40, 48, 56, 
  1,  9, 17, 25, 33, 41, 49, 57, 
  2, 10, 18, 26, 34, 42, 50, 58, 
  3, 11, 19, 27, 35, 43, 51, 59, 
  4, 12, 20, 28, 36, 44, 52, 60, 
  5, 13, 21, 29, 37, 45, 53, 61, 
  6, 14, 22, 30, 38, 46, 54, 62, 
  7, 15, 23, 31, 39, 47, 55, 63
};

int8 FiguresCounter::s_whiteColors_[] =
{
  0, 1, 0, 1, 0, 1, 0, 1,
  1, 0, 1, 0, 1, 0, 1, 0,
  0, 1, 0, 1, 0, 1, 0, 1,
  1, 0, 1, 0, 1, 0, 1, 0,
  0, 1, 0, 1, 0, 1, 0, 1,
  1, 0, 1, 0, 1, 0, 1, 0,
  0, 1, 0, 1, 0, 1, 0, 1,
  1, 0, 1, 0, 1, 0, 1, 0
};

uint64 FiguresCounter::s_whiteMask_       = 0x55aa55aa55aa55aaULL;
uint64 FiguresManager::s_zobristColor_    = 0x929d03167393eb95ULL;
uint64 FiguresManager::s_zobristNullmove_ = 0x423fb7585a2239caULL;

uint64 FiguresManager::s_zobristCastle_[2][2] = { {0x3cd26f386a9a70f8, 0xaf9e7fa97746b4a9}, {0xa4d594719b454679, 0xf791bf94729ed437} };

uint64 FiguresManager::s_zobristCodes_ [] = { 0x21c31356ab938c26, 0x4d52c6acefbaf9ec, 0x8af0d2462a35fd4a, 0xc32708cd3f32424c, 0xb8f25b5802933eb2, 0xa74b4465b58e6a78, 0xd9f7cafd84912b9e, 0x396af4c513f6b7d7, 0xdf4482842c103d4b, 0x9796577f6d2bb1a1, 0xbecb3b32173abd63, 0x75996d281d4b4bdb, 0xea73fe137a062e50, 0xec87dc8d3bfb6c3b, 0x4cf65c4ca1a22eef, 0xfdb340f51d19b51a, 
  0x4b5fe1b98d3f32bc, 0x10df51998993cea7, 0x548d25a43f7a8dde, 0x94cc821ef9f42384, 0x6cb997138477e279, 0x983902b71013fb24, 0xc2ddfe80dc488d60, 0x53b87424c298da81, 0xcc9d90ddb1dd84e7, 0x9cf42a0dc18ec51a, 0x625ba63874929d5f, 0x877b9d2184a3f0d3, 0x011f91c6faebe88e, 0x7b2472532724f5b3, 0xfe20b2e33e3d0fbe, 0x43ae60ae71ece561, 
  0x83a72ed75b9bb800, 0x9372116e0944ee77, 0x2b6826e07d784830, 0x309eb53a87c4e1da, 0x121a6486b71e635d, 0x038badbcda4c0e46, 0x27af1e73b162428f, 0xbd8df0ae31880d14, 0x84dc59c4cf31f029, 0xa3b2f7505d2aa612, 0x3606e1055a94c5f7, 0xeb97b182313616b3, 0x416b6c782bf50fe2, 0x63b6c72abd8f8282, 0x327af892832d04f0, 0x0b4a61d09b199abd, 
  0x44685a8773023da3, 0xae58705fd2473b01, 0x76f0def9f6176150, 0x0f83d59cdd3eae8a, 0x3bd4da7a3149fd6f, 0xf025c8e6bb799225, 0xc7592e217049f51b, 0x4f60800ead91ca00, 0xc8ea73c12c7d5958, 0x40fb8b61c8268982, 0x5d594f30886f9853, 0xf075feaed8b58ba3, 0x3c2d22e929422f1e, 0x277e658d30a0cae4, 0x2926d72c64949588, 0x6d5ffefe44553297, 
  0x3dcf6676541a6b57, 0xcede919c86556c05, 0x76b13af6dfeda963, 0x85529f69349d5559, 0x02f5e34adee2cc33, 0x9c9772231e538779, 0x91540499f8846727, 0x9ee3d2ecbb0c50d4, 0x684eb87a88ac5b00, 0xc807801166df24cc, 0x6ac6c1632110cd14, 0x3ad482a0d3bdbd7c, 0x2cb8f5caece70a52, 0x8e08c6a553a6efc0, 0x2ef7fad093217c80, 0x8651cd7915df06bf, 
  0x637f30fadd569879, 0x51af02c2d177eb90, 0x47962c8952d8d8c7, 0x28e85dc2ed8221cd, 0x2ca5707c9fb0e136, 0xa1e7f9b0484861a7, 0x18ca8dbdc9e5f3ad, 0xa8ef67462076ec8e, 0x2fa6b960c77688f8, 0xfd524d38c70c28e3, 0x7ba8a36d3f7d7cd4, 0x5eabd77699781998, 0x8518ff763226aa6b, 0xf49d3a9549c85e80, 0x78dcee7d0448f8d0, 0xf2c9a664a5cb4c47, 
  0xb691db80c55977f9, 0x123ae78f54e3c816, 0xf89f4d003a8f8891, 0x8e6bed39a427eda7, 0x0881ef16f14cd0ea, 0xd724e154a783b677, 0xe67bb9621f6df5b4, 0x6f2c85858d467cfd, 0x36dde3f433eb34a2, 0xf05d87231882aaeb, 0x444b6baaeab0db4c, 0x30ffc077306d10b4, 0xc6807e8158d33567, 0xfa1a1f90196a904c, 0x9f3658f28b9a6aa9, 0x58dca488090671c0, 
  0x9ea22c28a387db09, 0xc40dbd8ea850d2de, 0x6dd99a659d3e30f1, 0xc248aefbfb50d217, 0x32656e82f20c3368, 0x2c8206bae3bae1ae, 0x1a002c2005a096c4, 0x91d9fec212ec2e59, 0xd95f9e2846031169, 0x2e92b414612a7233, 0x12b75b02e1e940e6, 0xacea6a5ce8437de6, 0x55e66d46b65ba5db, 0x293bb4214e3b16c9, 0x8ebd71ea0beaf721, 0xf3d9cdb9d9624074, 
  0xdd9ab53502d74d72, 0x160353db186d8b96, 0x413fe1c570aa5e08, 0xbaa089354811c116, 0x7e98e289c1354028, 0xde1af6379146d186, 0x01307ce338dfebe2, 0x40956a014d7e95db, 0x2b1bd192a3340661, 0xe1a9bc38a9edab81, 0x200b5bd5fb008ad5, 0xc8bdeabf370543c5, 0x25c0644de565a471, 0xcb1ebbeebe6082bd, 0x43ebca08d51aeb31, 0xb9fa7f3fdd0ae468, 
  0xd06302a539d9359e, 0x0614d928baf30be6, 0x32993b79d4189152, 0xc4bb84fb9818ca10, 0x1cf925cbb17e2393, 0x9944b57c82ca6d24, 0x70594a8b19509636, 0x514425ae5747a05b, 0xd8aad5fdb704386b, 0x4b0db3f25917135c, 0xf9a363d2efdf69ed, 0x8e8f1673ed6cc9a9, 0x5479b91697d64a90, 0x7e6a9d1259ee11e9, 0x13612ebfd9dbe00f, 0x46baab25687a288e, 
  0xe70df6bb6347ed42, 0x5de9fc7b52ded54b, 0xdef8a5a3f8d6e3c6, 0x65cfeb97f9532388, 0xdd4ffa41a4e0e5d1, 0x92019132b89f7418, 0x749704647566d325, 0x117fa0803bab7366, 0x65e46c608eede317, 0xfc775d0706bdac20, 0xe2d0984fe0ecc6d7, 0xfc9301d5e24bcba7, 0x548ff7dc33d70ee0, 0xfc7bad52dcd7808f, 0xfd0ffea3909d584e, 0x2a85de7fd6205e35, 
  0x94034e62397bf8a8, 0x1d8204c9abfef93b, 0x6928ca3f21bd2b9e, 0x1bb7b75e42b9557c, 0x09c239ca41339e3d, 0x2c73d0cd311a1613, 0x9d6d4f9cc4d26acf, 0xe32e5ab0fc20fe26, 0xb4c333e260a52f0e, 0xafbb93e204a9d6ee, 0xc5a2f243e6823ab7, 0x468c11180af38617, 0xf3fe025884418be2, 0xa9d2144d432f43d6, 0xd512b90b1eb0cbd2, 0xb1d21cf7fe872599, 
  0x4dea13dcf768dad3, 0x7d630d8556f6d7fb, 0x2fe7f44494cfd2f8, 0xd97b71854c12c0f8, 0x223ca2ae5e9ccd65, 0xd56b31acebe542e9, 0xc568162777d5c50b, 0x7f1b8b63a3ecd684, 0x30702354ba120afa, 0xa27b8d1b7cdc926e, 0x098a70d6925eb9aa, 0x0d8567795df18d66, 0x2ff1f8b3a9c64146, 0x3feb49cac7d09026, 0x125bce3c2d12ac26, 0x162d980ea304d415, 
  0xe436194d400662bf, 0x2de15ccee4456d99, 0x0ced432e51cad313, 0x8de75498557d5bed, 0xb618c72202cbb805, 0xdac6d512dd708a75, 0x92fddc38863c6647, 0xea62820804078190, 0x195f1cf790a03df8, 0x9a6420ae5c7532d5, 0xc81df813b08569ce, 0x0416d8b4afa8bebe, 0xfe94d8cc27c2b895, 0x07d0b1f51e1d9b6c, 0xb3e45010913e15e8, 0x30b9265469c0aedb, 
  0xb5103689a50405c3, 0x6c2ec1504f79102c, 0xfd2cc914d3ef4c2f, 0xf463c9fd191afb59, 0x67e4c543a8f532e6, 0x7cc5a01af8f8892d, 0x55263d2fd3413613, 0xf3ad4c45e0cecb2c, 0x2572ca75c78e3f30, 0x3ee62fb34287c8e7, 0xacd1aec33eaf89d4, 0xcec5d31f5d1d6bec, 0x30e03632efffdd9d, 0x645a150c939546fc, 0x0568ea69d97a1833, 0xed8da267e55e2b32, 
  0xa0c42c80691e89e9, 0xf9da1fd74a228284, 0xc0732301a1c27fdb, 0x041967a6405015b5, 0x13f17f5ff3b5c64a, 0xe9929106bb192003, 0xc4779c927d63c8ec, 0x399e4115e2f099a9, 0x9adc9fd03e3c34c0, 0x46d331f9cbf6c37f, 0xbeac8568c7e241ec, 0xdb849d1cbba604b0, 0xc0fa73d9ec5ee91f, 0x21ca1280ad3c80df, 0xde95271d32c171c7, 0xcd5835156de11982, 
  0xce87e133846beb18, 0x9c0a6cf12e711428, 0x62f0df4e50acd581, 0xd14bea6c99d58773, 0x49aa35fcb70bf72e, 0x5ec5d7a42204b0ba, 0x13ea98909bd61834, 0x629f4d4a0baff4cd, 0xf085595a536f04e1, 0x81f8ca600c6a5cc7, 0x22daa2c3bc2f103c, 0x616f8c25b236224b, 0x1946711362cfec13, 0xdd93c3510af040bf, 0x6fd4519b881cef49, 0xbb2563ac36da3180, 
  0xbb28eb0405aa6838, 0xd6c2c76ce15887c2, 0x79949d1898c30198, 0x8034cb84deca6532, 0x89809dbbc943c8d0, 0x4e563ae2e5301e72, 0x7f743bd74f24238c, 0xee5efd257b6460b9, 0x043d2bba8fdb7f11, 0xe665815ba1ef5017, 0x2fe1cae1e6572215, 0x6540eec1be8fcdc1, 0x633e996b346f98f6, 0x214c90cb7ed09a21, 0x2e9582ce72a66bd1, 0xe968751e14851f8c, 
  0x96456e417c33ce66, 0xf2dd8b94baf01881, 0x12f1f8c18e4769f8, 0xa669ce141a103382, 0x07bf1f7275eba853, 0x7b1b8971dc6032e2, 0x50119a8c7f9e5f3e, 0x28a728762636a14f, 0x9407c6653e295da9, 0x7edd37cec6167718, 0xcbd3cbb8bfbc6e0d, 0x8691eb098cfd2e3d, 0xac8f5f70dc2a61d0, 0xba3019d611e251ed, 0x8d47c127ed4d6898, 0x11597364a6ba71d3, 
  0x6ca5a12d334514d2, 0xe5c2f9bca80d120d, 0x0c0f57b57800228e, 0xb7e9ba07c4b1b854, 0xda0f1832c9faca37, 0xf0eb02ca64ae47fc, 0xc913e4f36516b0d5, 0xaa3abc49c31d36c2, 0x264993f5f2c3d527, 0xd5f631172efd5324, 0x6acc22ae2943e2ed, 0x2b90801e6643119d, 0x126f5b05035587c8, 0xfefdd697ae01f1a9, 0xd07ad2c692f7fa6a, 0xae491e5f16fe6549, 
  0x93ba8e4681818851, 0x07517972dcceb49f, 0xe0745828d79cbe16, 0x65c7e82b1439b960, 0x802e226d81ef591d, 0xf8d59a233d6e6497, 0x0d030bbe0e4fbb29, 0x36a99fb666094eba, 0x98830483093da716, 0xba05d90f77c9e948, 0x1cea09e7713f6107, 0x306f60ae2b77c0b9, 0x22cb50b434669cc4, 0xf4e974404cae1c62, 0x6bc8402113b33012, 0xede1f5f6360839d5, 
  0x45ab4074f1712583, 0x2fd61b399c90086f, 0x02b3ee655a4fed25, 0xfe47fbe691fca8b3, 0x24141ac482a64cbd, 0x985787cf65d01cff, 0xe7d212cd165e9ebb, 0x0c1914e34f63d4ea, 0xb098fe0bebabc3ba, 0xb6bec5557a8ea97a, 0xc4a39401f48c1de2, 0x47f6a748ea40c5aa, 0x0fad052db793e30c, 0x7a8fd6803c1243e2, 0x127255813902b11e, 0xe367f83a2b9a6b59, 
  0x7c056995af6bcb77, 0x40ef60efe2e88bae, 0x703f4f8369e571dd, 0xc5e1e926d4f65c83, 0xe088a9ed92e89edc, 0x16aacdc66314e6c1, 0xe18ac2372b9471d7, 0x66d2739aca5a6fa4, 0xf31098e9d4c8078e, 0x6cd305eae6b9dda1, 0x04f900754229a916, 0x165e968cb57ad432, 0xc5719265aa43e985, 0x94b177f033d7602a, 0xf0f46a22b5d43162, 0x831d163a0c2e2bb5, 
  0x4a1588f6f561f8bb, 0x0f4c864e13fbe0f8, 0xd1796d2d2e857267, 0xe7c0580823195935, 0xc8c3a2cf3acd9404, 0x230f21a956f89b95, 0xcc84b0491393ef6a, 0xf2465693299b50df, 0x947b23954c87ca50, 0xb34a2ca39bbc7089, 0x6098c85abe6afa0a, 0x7548aec135f9af92, 0xa944ccea5373b25d, 0xb9bd4783b3ec0fd0, 0x701bfa769ac70eb3, 0xab2a7596feb0544e, 
  0xfd2bf708dc34456f, 0xccd567b88107d518, 0xad380dcb69f88623, 0xef2ea0b8fffc52d9, 0x022533284301b95a, 0x73cbe9410d0725ac, 0x82289b5e814b1766, 0x8effec56bdb77917, 0xd4b8a9edbd3be943, 0x127a363b8faaad63, 0x3d5b612581296dac, 0x57de5667dc04619c, 0x2ce45b5b581ce5c5, 0xa66ef778a7a10483, 0xe30a6bae274e187f, 0x88aadf26f74c792e, 
  0x72600b5ed83f44d8, 0xb0f2c9f80d5a8554, 0x5bcdc7cc9e72737f, 0x57f577474d12fe0c, 0x19786de2085182c6, 0x0b0fd8c9fa348d10, 0xd03984e56a09d734, 0x862d2cffc35ecbd9, 0xa75b015472a60137, 0xeeca2261c14920bb, 0x77ace045c41fca47, 0x809dcfdd3c581a47, 0x6fbf100046a97abb, 0x26bc595775cef0e3, 0x3cefcc80fb551e43, 0x69181935d1db617e, 
  0x18b261522c86d140, 0x625c56fe15afb590, 0x7a99faa0144e0138, 0xafb5f6b71682ca0e, 0xc8c8bca48317732d, 0xd973bb8cda67c33e, 0x4f497bf0633e884c, 0x3eab75541a38c595, 0xffa40147a7be0859, 0x80f5cf619c8c0c0a, 0xc1be16344d82359b, 0x78e57df42ebc5f2f, 0x91493782e16112f6, 0x045de6583f6204fb, 0xf670a570ed8c7951, 0xf86cf6b0c3fd90a0, 
  0x37421405b89eff08, 0x49b3515efb853f85, 0x0c6802c1064b6fe0, 0xadf5cb23d6f199ae, 0x9c495734a2fbb583, 0x8fd2f2528e0b7e5e, 0x21f9a8d1fe658be7, 0x13dbf0133c71d030, 0x03c6c23b30476dd9, 0x5af8b6aa7e22255a, 0xfcab0a21acd8240d, 0x00d60535b7afd321, 0xb4afe2748d9b4172, 0xeddf7bfb5dfd1601, 0xaac22a6c482f6fd8, 0x30c75ef0869ab423, 
  0xcc80d76570e1c622, 0x8d7144d27df242a1, 0x0835cefab662a518, 0x973ed8f348d43cf2, 0x2baf5765ccfc3aaf, 0xb409f6c6e34fe4a6, 0x6f4c4102d362cab5, 0xfe6df00cc2e9a8c6, 0x83da524b4f8030e8, 0x2962f8c1ac197de8, 0x8ca80116180a0a15, 0xafde11ff83b692c9, 0x33f1fa81e6918970, 0xc35c78ce787fcb44, 0xb574d74ae2736b60, 0xbe4a9c83d8e964bf, 
  0x9c70dab8ba7bd4c9, 0xd36c8bb9408df81d, 0xe1663073b1090b8a, 0x812f63df9222cb64, 0xf2b80d43dc036c4f, 0xb67dbd196395c96b, 0x6c008d3705386581, 0x5722a7c3fd05b6fe, 0x6906bbc7d396f5f6, 0x3d5e98d19e151ee4, 0x87941083146af308, 0xd7ae600b11f8e4dc, 0xbfc1358f2e2b14c4, 0x6c30eeb951570e78, 0x20dff4fdef54e032, 0x7fcec5d467c8dbdf, 
  0xfda97ec78b975a75, 0x4784b9428588f10b, 0xe15ca2ef5a8d4ac5, 0x94c48b8f0fec4e87, 0x2fb8ccc4cd5e96b5, 0x1e353c826f15e933, 0xdc850c79bfe69849, 0x70b2fa9791f675de, 0xad214225bb41628a, 0x5a05aeaf439d8d07, 0xbe0777489ba63a6a, 0xa9f2c168277e6a20, 0xed8562d6f3212e07, 0xa5c37725102c03ce, 0xce17d7e76c181e46, 0xe39db66939b594bd, 
  0xeb0ec4d539f03c2d, 0x85b2ddd787c09032, 0x2b4c51672463be9d, 0x06f9ff2eb0689867, 0x26f693cb73176754, 0xb155b86a44f9982b, 0xae5e6d5411210dd1, 0xb5525de3f8a70e5e, 0x4dec8782b4283d66, 0x3d5b09b476b839bd, 0x038e456d3eb2bb9e, 0x1ccb157895f47876, 0x7dcc9a42aa99925b, 0x4b8ddd858d9916e6, 0x3ebf601d172bc4dd, 0x49e913e28fb1d9f0, 
  0xf3a5e124e0ba9a13, 0xf87e7d596939af4a, 0xd14ade9b037d3ddd, 0xd1934e0f4d3affaf, 0x4f4dbeb9348cef24, 0xe66dd3544abcdcb9, 0xb3292aaa0f584cc2, 0xd7e6fdbd2efdb25b, 0xe443ec914ff14eca, 0x3bb1cd7e8eae0f31, 0x75c935b1b20cda8a, 0x5713175af7192f46, 0xbeaff0aaceb039e5, 0x9abf0d74ef8acd41, 0x99e7c4d56c48a2bd, 0x99c84489461bc854, 
  0x1043f7d656a3669e, 0x411df14649a19614, 0xf9b7dc839602ff9b, 0x82d4b328ff7a0bbd, 0xb7c133abf59c11ac, 0x7212686c37517768, 0xf92b4abe9f0836ae, 0x96aa30b1397928d3, 0xf3b2b851b0dd1e83, 0x53f3f117afcd4769, 0xc520a20d1a57abde, 0xce3f700daf34d756, 0xb3740af8f5afd66d, 0xcc1640c488d0d0c7, 0x580abe28cf278c07, 0x0f3e40260393796a, 
  0xed5b8e87224c61aa, 0xd6d70aea72087670, 0x556188d331c19b1e, 0x211330950ebc3418, 0xb84b7bd5607672ae, 0x6adccdc67cdf9ad4, 0x1bc57eb64fb5b632, 0x040ab3e52dc34293, 0x6b8e0f0330b3439f, 0x4fe23e83664f2f1d, 0xea864d46b959b57f, 0xd42e9ea08f85aaa3, 0xbace51aa3f1d004d, 0xa3bb74616b83b1c7, 0x7b7815d86985b0db, 0x1ba272ee5fa4d66a, 
  0x3e1f88ad8f24d438, 0xec78cebf9a5da38b, 0x8707416eaec56a2a, 0xd81be20e38c208a4, 0xd0f4f58f52a50c9d, 0x44ad8bf9fd084418, 0xa0c62d080206fa5c, 0x6661c2670c3a3801, 0x5d1c821f1ee65823, 0x458ac8eae11f4b4e, 0x842e94d2ab7b7a3b, 0x59f48b58ce45b08b, 0xc1490ac21d6da8ce, 0x5f5c307f68bc701f, 0x9b38f129661a4c6e, 0x28270821fd18ff49, 
  0xd658d38d9bf937e0, 0x5ab7eba6e8fb43e3, 0x1b89c23fcaed75f8, 0x5f41683021622faa, 0xfd30863557870785, 0xecccae8340a3782a, 0x1e03341aa8e6b58f, 0x648d5dae8095a094, 0x13b5d7ebdf9651b9, 0xfdc26a0c40620eae, 0xf76dfa9a972bd43f, 0x78411d2094a58efb, 0x612305f6ac6c2e92, 0x7d525e806531aed7, 0xd9b508dba1d85408, 0x14cd14629f429bd7, 
  0xa60c4c9e3d3bd713, 0x663a25d402a85545, 0x6f0d88b0015f02d0, 0xc77defd0dfb54b2e, 0x88a77b34c5ca0667, 0x149542392cf594f1, 0xd88b23e29e6e3f93, 0xd8001bd8020e01d4, 0x1383217e8fbabac8, 0x50537fb27cb270be, 0x0c8fb424766350ad, 0xdf3602d28193620d, 0x858d4357c2ece216, 0x0b5020d0f76b22f0, 0xa565ceb187406098, 0x04832be0c61e62b3, 
  0xffc7ad9448d31401, 0x89fd217e87ecda79, 0x70ee71522266ef1b, 0x871c447b8c722465, 0xf3f88caa00b174cb, 0xe48a3ea1a10a495d, 0xc93e1d5d2df99a79, 0x2fa144719ff1ec92, 0xf78e3033be3bc1c4, 0x4b533cae54c8fd50, 0xd95473189bd89d98, 0x88d6d7cfa28c0740, 0x9391b6c66facb1ac, 0xb9f717f9f8daeeac, 0x64fb9d77f1f4567c, 0x0d32369703625723, 
  0xbdcbf559098aacc5, 0x95677689056362ac, 0x5edc8e7a3bfc27f5, 0x270622af732657e9, 0x8da4e5775eb4c03a, 0x5a3ee9f0bf424677, 0xe98ed04cbecb7189, 0xf579cb557883e2e2, 0x4ed4c91a74ccfbaa, 0x86d55434012289a3, 0x9b1593bda29b4b40, 0xd2f8c61224e4f15c, 0xcb06b15607eb607f, 0x64b693c757a3fe28, 0xa51b05c460f3ccce, 0xf0836bd6902efd43, 
  0xaf8b73bd574342d5, 0x6a96fe685951bc86, 0xc5bec8cf12f591bd, 0x3ae09ab3519277db, 0x552a497dfb3cf77e, 0x7f82650109f6e04f, 0xa9f331af1b5b2210, 0x668f9095d8bfc911, 0x3e644e616170d69e, 0x4789930582255fbb, 0xf5429f381f472f58, 0x5356ee75c6efa0d0, 0x9cb569b584f6326b, 0x8efdcdbb5ea06822, 0x35133699ff22f35d, 0x4f2a17cebf6bf06c, 
  0x00a757c31284b565, 0xbd49609e85c96cfe, 0x8aade3fb216943e5, 0xd2497ac9b13834f3, 0x815e0689359555e4, 0x93f3b3656b497c5e, 0x455de62f0ac64378, 0x933933480883b875, 0x2077d588b320052d, 0xae94bcf816d29835, 0xcb80c6046f1af10a, 0x8b2851e31891da2a, 0xf0f341ef78ed1607, 0x6c34aaab5886581d, 0x679cb8c5604133b5, 0x7e8e441fdc7ec158, 
  0xfec13961c4c0781e, 0x3b33248c8acf30aa, 0xe290a75484c996b5, 0xd1c4ba874820b083, 0x4f0bb9b596e17fdc, 0x580172a6d79ae8c4, 0xa7b7b2501e5a3426, 0x1623e0a6b19516bf, 0x3ff0957cc18563f5, 0x4af160a4b2f1f9f5, 0xe13c99410fea6f21, 0xbb032de661e04b09, 0x11de520fd371f355, 0xb957a34c5da633e9, 0xce8979654beff1ad, 0x43814e4864c3ad14, 
  0x900e1a219feb504a, 0xb4c31463e34020bc, 0x336d2f7c625216be, 0x7b01dd11a1c3eb94, 0x530d0a5c3733d2b4, 0x939fe766285c5258, 0xaf14e6cd00055ab2, 0x64ec3c6a47a7278f, 0x62ced5c740b2f887, 0x2041c5dbdf5d9f20, 0xa97aa66c9eefad19, 0x009f39b59b756055, 0x9b2a4240494b6fb4, 0x1feb5e6e3a4ef695, 0x18472f133c607543, 0x169352411ea8b626, 
  0x157c5553099b0bfe, 0x56ef0ada309a4f47, 0xf36cb9a578d2428a, 0x921d5e5b11fc1fc8, 0xd81d6603bd17231d, 0x6d53b66fb2dd74b4, 0x5e1edcde5d7e3419, 0x354e337f6f273d7e, 0xf77564d2822a6acb, 0xe9344287556b3d94, 0x87e057de69b6dd3b, 0x8ccffdfe379702a7, 0xec7c732b80a00dcc, 0x87718874ed3e4f53, 0xf4846f55c2480356, 0xd057aab500f5a1f5, 
  0xda034c5b7e3b7ab8, 0x5e3aae0c11a8872f, 0x424549a49c0f1df6, 0xee697e5bbad71cd4, 0x6d30d44fa0fe15d5, 0xd974226aeb950120, 0xdd7d3596a77547a7, 0xf5027f6e98f85218, 0xd418b5f9d74ca1de, 0x5ff4c8fb78c80542, 0x39d3669fa88d3d7f, 0x51227176214d7f67, 0x6331be34e37368c2, 0xf2263fecf153132a, 0x5d2d1ec7fc10142a, 0x97c3f31074584459, 
  0x2189cb255e46ea83, 0x62816c5d4f1f63af, 0x301b448282cd8cf8, 0xb23e527b4ecd97ae, 0x6d6fac3c54c2e8ad, 0x8d65481d475b7b4d, 0x1dea50b7954654b3, 0xa6287f1a80163d20, 0xf9194c3531322de8, 0x6ee867765c284db2, 0x0c5fd53b1af8de5a, 0x6acfcb92e10b43ba, 0x168fa3f4f31d32be, 0x84f862a8fbf6cf32, 0x6af2a10bcc5c3500, 0xc707c3c0db478b67, 
  0x24a3bf56ed9a3797, 0x4bf84b4020cfdd35, 0x5c4ccd30f3f8b79b, 0x3c42c1c280450a61, 0x0c0b57163063ccd3, 0x9d3c2d7bc726b641, 0x84b3c658b300a35f, 0xf189f9adb39e99a4, 0x9c7d9cee6dae0374, 0x400909614da706f9, 0xe8f17f2c57898f48, 0xa50b31db42d3e3d4, 0x7d5117f5abf960e9, 0x4a73bb1a04861490, 0x33857b95c02bd694, 0xe01629a62623a167, 
  0x568932bed653951d, 0x19be4a39edef7d00, 0x3b0038c92473d193, 0x6e452872a62b2f1d, 0x3371f6367ebd2b42, 0x2c18a89d1d178fbd, 0x77772e55dde63459, 0xedeaf60b18cf1b20, 0x270f36e5580ec04c, 0xac2c78ec4be1f3ff, 0x856949d990938878, 0x3067866ae894afc8, 0x31c00028b0edb6b7, 0xe5d8275cc1598b44, 0x947da20ea783daf7, 0xdf03e02aaa41f9e6, 
  0x70bee1ad39dff2fd, 0xb0b2ed6ab20a435c, 0x0ba81854ed9f751f, 0xc27c97fb0f44cd39, 0xae5ec29e1a980fa6, 0xf892c0820cb52c63, 0x4d3b40486da1bcd0, 0x0ec5e1ca96cac9a5, 0x118aa01e4bdb11d4, 0x89f1a4fd015b40c7, 0x2cd07598542e3250, 0x5c2ed9d5ebd26ab4, 0x718f8c8baf07763b, 0x2e30157dc642e07f, 0x6d075fc13c33a67b, 0x566f3177beaf6d2e, 
  0x5ea8d81c7b016dec, 0xdc91867d4c7bfd42, 0x842b7c141c27e76d, 0x3166b701e88f38b7, 0x864e025bb01ad74a, 0xb320ff9d935a0032, 0x7954734f8d967ae0, 0x5583266df294cdc1, 0x00fdabf4ff6d271d, 0xa74c502d1c05e68f, 0x0fe827faf8063bf8, 0x266f4af1cdc15d08, 0x933a627ea6300247, 0x67b0d8328eb3ed35, 0x08d3dad188191685, 0xe8e49040f1019a0c, 
  0x3ad3f37fbff0cc74, 0x78f89849867771ae, 0x5ee1230173fda77c, 0x03a3aa922074bb66, 0x475ff3901ad4072c, 0xfffc6bab28b7c626, 0xad82658035b80230, 0xfb6444c0eb6854ed, 0xf3ce7d8928184db5, 0x1931f31028ad1581, 0xfe57929f022ed999, 0xdd3b33eac38b97fd, 0x6aba9463fa8c22eb, 0x0b05e34b2f847d3d, 0x238750f819b61d55, 0x6bac12d198baa830, 
  0x789a1f98ce4f3c1a, 0xff4f107e6639334d, 0xeda7e4ef19fe3b7c, 0x0d224034ef6960e6, 0x875c3f3f0802038f, 0x9bff252c2bf7623c, 0x1c95d0dea854ffb2, 0xf14ef5350c007f2b, 0xa4bb735c5fcd267d, 0xc6ef117c58b47334, 0x5df0ab25af413131, 0xd8016d661139df31, 0x5f08e564ee958e4c, 0x23c6ace80ee54921, 0x88b3ffdbcff8dcdd, 0x1fcb041c111320a3, 
  0x43586130dbfc1ce6, 0x880e935115e599d1, 0x45aa6ed5c477fb7a, 0x441e1ed95c8d93bc, 0x6624c2de02eadcf3, 0xa3703a5341393050, 0x69b4d17dda177e21, 0x09857a56c71dc8fa, 0xba13f872ec085843, 0x7a8738671ea7e860, 0xeb2b4d7c97d82f29, 0x4c4b0c7ecd2dca65, 0xd4f52a4a5775439c, 0xe7435b1a0f12c145, 0x7e2c651f8634d026, 0x676a795b19344d71, 
  0xddc5f8b83e7361ad, 0x1fd1281032d1391b, 0xea3ac3bf2348b7fe, 0x53965560931c8b90, 0x1608627bf0193e95, 0xbb5f1a246f7470cb, 0x6b456f1d47578ce1, 0xc6f8034208bdaf6a, 0x1f4f51fc18b73f9e, 0x9fe05a500a006fa4, 0x978106074c9013df, 0x334ae9095ce3a173, 0x7ff85f5dd7e3f1e4, 0xd1617914db60232e, 0xb8dcf0dcfd076c2a, 0x0a5196e621e1dfcd, 
  0xdb4fb3b25bb409f3, 0x4e2e4f09f6a45d0b, 0x998cccb0859c04fd, 0xc63546a603cb2703, 0xc837d75b48a08835, 0x180301dc9db05b1b, 0x65763e63b22c4713, 0xdcd84b1d7346777c, 0x4677f33e108e202a, 0xd10e923e9cdd27b6, 0xc25d42d14e73b572, 0x59eb422baa9d9236, 0x272a4ca63321d646, 0x1f44ede3ea9f2256, 0xfab01f158648ac3e, 0xc68aee28d790fd95, 
  0xc3b303470b95b4bf, 0x5dfed1f9b0c682b3, 0x9c9ffc092546e78b, 0x6917a0cac656840d, 0xb5271f0f1204be94, 0x1ad6f882ff2bbd95, 0x10fb22d083e60d2f, 0xd91f201b41f4ccd8, 0x6d366157f86091e8, 0x0f1d77b175a52c9d, 0x976a5a2dfc38d0ea, 0x80cfe561666a60be, 0x1e58b1625e127291, 0x16e6cd837b2b07d6, 0xdd282a1289046c84, 0xbe01ae7ed328334b, 
  0x667881057b4a9c4f, 0x95deacfa025478a4, 0x33cfe537ceb03d3b, 0x2f632d18d4377561, 0x06c991aa4435627a, 0xb2fe26eb46e4b869, 0x4e22c4b6f45eaaf6, 0xd0012f6311ab726b, 0x7508f0527c595c4c, 0xdfe08034f0ca97e9, 0x5e66f5dfa9465990, 0xbe5bed289e0aa446, 0x10bfb9b8db736939, 0xc7875d23284ce0f0, 0x296a3d3972c6b48f, 0xf8c6874f9d30ec4a, 
  0xbf17e9b2f14994a5, 0x695e2092a1767ac8, 0xf3606fc0a0b31a7f, 0xf0096b7a5bb73555, 0xfedbb5dea7524dde, 0x6cd258ada0850d44, 0xcbf5e19ff92815b2, 0x45b30a8353d8c1bb, 0xb254643d67d5657f, 0x8258cbb633ca53f3, 0xa09f8a64eb740df8, 0xbac725dfd8681c48, 0x4220b51a29e4f199, 0xe4aa305b567e29ab, 0xe724adf2904951db, 0x14c2054b2f4e28da, 
  0x90257bbbd0d208f4, 0xf894112dcf9d2674, 0x996795377f5263bb, 0xd048169d8c2e6739, 0x39cf687e208e6e7a, 0xba2e68417cf5f1f6, 0x6612e77b1c57a908, 0x61f07c6b5803e5fd, 0xa8458eaad0a20ed3, 0xce606c3c39cde99b, 0x043fc203eda0021b, 0x969b82ccb0a7f813, 0xbf3300d5046f1a77, 0x6246707ebb3e6b43, 0x75ffd1c074fbb867, 0x108b2e7c51415492, 
  0x25178477770742f4, 0x49dc531bae711da2, 0xbf7a89dd148bdd6c, 0x5202ae1ff65fd84a, 0x981c8d2f8bd79bfe, 0x65de9321c2b9596a, 0x083909972d0ed038, 0xc3b6703e35221d91, 0x9ed4bb00b53bd7e5, 0xbb39daf2a90f6ea6, 0xa7821550d2dc06f7, 0xba9ca995491660b3, 0x81c2dd9d662653f2, 0x372033b9f3ecb9d1, 0x3e65ec5ad03595ed, 0xc4d45e571aeef46c, 
  0x0f35f719a6ca718c, 0x51762f9ccd34c911, 0x86210a03a480cefc, 0x1de2fd882db3959a, 0x6979048f5d39d307, 0x89a2f352689f8712, 0x43eb31ed6a6482f0, 0x7a4e3ce283c5e404, 0xa93fde36ab25b1d5, 0x3676ec1b948a6c68, 0x255402b5c27a5759, 0x6dbd1faee70e174d, 0xa14ade9e5331fb76, 0x346e59a3160c1a2d, 0x77ec1b1fe11ba6f4, 0xe0c02f70f047774f, 
  0x0f2ead542362465e, 0x44e0937251ee70ad, 0xc069f9a68c102854, 0x8f80dfa10f2ddc22, 0x5421bde58adf3d63, 0xd32b43961151535a, 0xd4fbc40b73d33b89, 0x34d205c3c9807096, 0x54b852017a18af24, 0x960799f8b90456bc, 0x1ee105aef8631849, 0x1eb412ac7731af09, 0xded47cbf87d2618c, 0x5af2f8c69329ee28, 0xfd7940647248bb44, 0xd8270760c877ce6f, 
  0x682d8a5ceec998bd, 0x6c11ff8e012fb1b9, 0x59009ce2c2deb1be, 0x2ce803a50bd3177c, 0x32f91b13b79ce5cf, 0x335ed0bf3149d427, 0x05c8fa571a45d589, 0xe7ed0f347f147316, 0x26196732aebba47e, 0x3f8116fbd3bdbde0, 0x3f380080197bcfd8, 0x75c1bf0a26fadb6c, 0x2fd954b9a158aba4, 0xd2fd78c3d6d8a754, 0x55c362639884102a, 0xcac4e81320d95e01 };

} // NEngine
