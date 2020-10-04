#include <cstdio>
#include <cstdlib>
#include "MaxBIM.hpp"
#include "Definitions.hpp"
#include "StringConversion.hpp"
#include "UtilityFunctions.hpp"
#include "WallEuroformPlacer.hpp"

static PlacingZone		placingZone;			// �⺻ ���� ���� ����
static PlacingZone		placingZoneBackside;	// �ݴ��� ���鿡�� ���� ���� ���� �ο�, �� �������� ��Ī�� (placingZone�� �޸� �����ʺ��� ��ü�� ��ġ��)
static InfoWall			infoWall;				// �� ��ü ����
static short			clickedBtnItemIdx;		// �׸��� ��ư���� Ŭ���� ��ư�� �ε��� ��ȣ�� ����
static short			layerInd;				// ��ü�� ��ġ�� ���̾� �ε���
static short			itemInitIdx = GRIDBUTTON_IDX_START;		// �׸��� ��ư �׸� �ε��� ���۹�ȣ


// 1�� �޴�: ������/���ڳ� ���� ��ġ�ϴ� ���� ��ƾ
GSErrCode	placeEuroformOnWall (void)
{
	GSErrCode	err = NoError;
	short		result;
	long		nSel;
	short		xx, yy;
	long		result_compare_x = 0, result_compare_y = 0, result_compare_z = 0;
	double		dx, dy, ang1, ang2;
	double		xPosLB, yPosLB, zPosLB;
	double		xPosRT, yPosRT, zPosRT;
	//API_Coord	p1, p2, p3, p4, pi;

	// Selection Manager ���� ����
	API_SelectionInfo		selectionInfo;
	API_Element				tElem;
	API_Neig				**selNeigs;
	GS::Array<API_Guid>&	walls = GS::Array<API_Guid> ();
	GS::Array<API_Guid>&	morphs = GS::Array<API_Guid> ();
	GS::Array<API_Guid>&	beams = GS::Array<API_Guid> ();
	long					nWalls = 0;
	long					nMorphs = 0;
	long					nBeams = 0;

	// ��ü ���� ��������
	API_Element				elem;
	API_ElementMemo			memo;
	API_ElemInfo3D			info3D;

	// ���� ��ü ����
	InfoMorph				infoMorph;

	// �۾� �� ����
	API_StoryInfo	storyInfo;
	double			minusLevel;


	err = ACAPI_Selection_Get (&selectionInfo, &selNeigs, true);	// ������ ��� ��������
	BMKillHandle ((GSHandle *) &selectionInfo.marquee.coords);
	if (err == APIERR_NOPLAN) {
		ACAPI_WriteReport ("���� ������Ʈ â�� �����ϴ�.", true);
		return err;
	}
	if (err == APIERR_NOSEL) {
		ACAPI_WriteReport ("�ƹ� �͵� �������� �ʾҽ��ϴ�.\n�ʼ� ����: �� (1��), ���� ���� ���� (1��)\n�ɼ� ����: ���� ���ʰ� �´�� �� (�ټ�)", true);
		return err;
	}
	if (err != NoError) {
		BMKillHandle ((GSHandle *) &selNeigs);
		return err;
	}

	// �� 1��, ���� 1��, �� 0�� �̻� �����ؾ� ��
	if (selectionInfo.typeID != API_SelEmpty) {
		nSel = BMGetHandleSize ((GSHandle) selNeigs) / sizeof (API_Neig);
		for (xx = 0 ; xx < nSel && err == NoError ; ++xx) {
			tElem.header.typeID = Neig_To_ElemID ((*selNeigs)[xx].neigID);

			tElem.header.guid = (*selNeigs)[xx].guid;
			if (ACAPI_Element_Get (&tElem) != NoError)	// ������ �� �ִ� ����ΰ�?
				continue;

			if (tElem.header.typeID == API_WallID)		// ���ΰ�?
				walls.Push (tElem.header.guid);

			if (tElem.header.typeID == API_MorphID)		// �����ΰ�?
				morphs.Push (tElem.header.guid);

			if (tElem.header.typeID == API_BeamID)		// ���ΰ�?
				beams.Push (tElem.header.guid);
		}
	}
	BMKillHandle ((GSHandle *) &selNeigs);
	nWalls = walls.GetSize ();
	nMorphs = morphs.GetSize ();
	nBeams = beams.GetSize ();

	// ���� 1���ΰ�?
	if (nWalls != 1) {
		ACAPI_WriteReport ("���� 1�� �����ؾ� �մϴ�.", true);
		err = APIERR_GENERAL;
		return err;
	}

	// ������ 1���ΰ�?
	if (nMorphs != 1) {
		ACAPI_WriteReport ("���� ���� ������ 1�� �����ϼž� �մϴ�.", true);
		err = APIERR_GENERAL;
		return err;
	}

	// (1) �� ������ ������
	BNZeroMemory (&elem, sizeof (API_Element));
	BNZeroMemory (&memo, sizeof (API_ElementMemo));
	elem.header.guid = walls.Pop ();
	err = ACAPI_Element_Get (&elem);						// elem.wall.poly.nCoords : ������ ���� ������ �� ����
	err = ACAPI_Element_GetMemo (elem.header.guid, &memo);	// memo.coords : ������ ��ǥ�� ������ �� ����
	
	if (elem.wall.thickness != elem.wall.thickness1) {
		ACAPI_WriteReport ("���� �β��� �����ؾ� �մϴ�.", true);
		err = APIERR_GENERAL;
		return err;
	}
	infoWall.wallThk	= elem.wall.thickness;
	infoWall.floorInd	= elem.header.floorInd;
	infoWall.begX		= elem.wall.begC.x;
	infoWall.begY		= elem.wall.begC.y;
	infoWall.endX		= elem.wall.endC.x;
	infoWall.endY		= elem.wall.endC.y;

	ACAPI_DisposeElemMemoHdls (&memo);

	// (2) ���� ������ ������
	BNZeroMemory (&elem, sizeof (API_Element));
	elem.header.guid = morphs.Pop ();
	err = ACAPI_Element_Get (&elem);
	err = ACAPI_Element_Get3DInfo (elem.header, &info3D);

	// ���� ������ ���� ������(������ ���� ������) �ߴ�
	if (abs (info3D.bounds.zMax - info3D.bounds.zMin) < EPS) {
		ACAPI_WriteReport ("������ ������ ���� �ʽ��ϴ�.", true);
		err = APIERR_GENERAL;
		return err;
	}

	// ������ GUID ����
	infoMorph.guid = elem.header.guid;

	// ������ ���� ����
	infoMorph.horLen = GetDistance (info3D.bounds.xMin, info3D.bounds.yMin, info3D.bounds.xMax, info3D.bounds.yMax);

	// ������ ���� ����
	infoMorph.verLen = info3D.bounds.zMax - info3D.bounds.zMin;

	// ������ ���ϴ�, ���� �� ����
	if (abs (elem.morph.tranmat.tmx [11] - info3D.bounds.zMin) < EPS) {
		// �»�� ��ǥ ����
		infoMorph.leftBottomX = elem.morph.tranmat.tmx [3];
		infoMorph.leftBottomY = elem.morph.tranmat.tmx [7];
		infoMorph.leftBottomZ = elem.morph.tranmat.tmx [11];

		// ���� ��ǥ��?
		if (abs (infoMorph.leftBottomX - info3D.bounds.xMin) < EPS)
			infoMorph.rightTopX = info3D.bounds.xMax;
		else
			infoMorph.rightTopX = info3D.bounds.xMin;
		if (abs (infoMorph.leftBottomY - info3D.bounds.yMin) < EPS)
			infoMorph.rightTopY = info3D.bounds.yMax;
		else
			infoMorph.rightTopY = info3D.bounds.yMin;
		if (abs (infoMorph.leftBottomZ - info3D.bounds.zMin) < EPS)
			infoMorph.rightTopZ = info3D.bounds.zMax;
		else
			infoMorph.rightTopZ = info3D.bounds.zMin;
	} else {
		// ���� ��ǥ ����
		infoMorph.rightTopX = elem.morph.tranmat.tmx [3];
		infoMorph.rightTopY = elem.morph.tranmat.tmx [7];
		infoMorph.rightTopZ = elem.morph.tranmat.tmx [11];

		// ���ϴ� ��ǥ��?
		if (abs (infoMorph.rightTopX - info3D.bounds.xMin) < EPS)
			infoMorph.leftBottomX = info3D.bounds.xMax;
		else
			infoMorph.leftBottomX = info3D.bounds.xMin;
		if (abs (infoMorph.rightTopY - info3D.bounds.yMin) < EPS)
			infoMorph.leftBottomY = info3D.bounds.yMax;
		else
			infoMorph.leftBottomY = info3D.bounds.yMin;
		if (abs (infoMorph.rightTopZ - info3D.bounds.zMin) < EPS)
			infoMorph.leftBottomZ = info3D.bounds.zMax;
		else
			infoMorph.leftBottomZ = info3D.bounds.zMin;
	}

	// ������ Z�� ȸ�� ���� (���� ��ġ ����)
	dx = infoMorph.rightTopX - infoMorph.leftBottomX;
	dy = infoMorph.rightTopY - infoMorph.leftBottomY;
	infoMorph.ang = RadToDegree (atan2 (dy, dx));

	// ���� ������ ���� ���� ���� ������Ʈ
	placingZone.leftBottomX		= infoMorph.leftBottomX;
	placingZone.leftBottomY		= infoMorph.leftBottomY;
	placingZone.leftBottomZ		= infoMorph.leftBottomZ;
	placingZone.horLen			= infoMorph.horLen;
	placingZone.verLen			= infoMorph.verLen;
	placingZone.ang				= DegreeToRad (infoMorph.ang);
	placingZone.nInterfereBeams	= (short)nBeams;
	
	// (3) ������ ���� �ִٸ�,
	for (xx = 0 ; xx < nBeams ; ++xx) {

		BNZeroMemory (&elem, sizeof (API_Element));
		BNZeroMemory (&memo, sizeof (API_ElementMemo));
		elem.header.guid = beams.Pop ();
		err = ACAPI_Element_Get (&elem);
		err = ACAPI_Element_GetMemo (elem.header.guid, &memo);

		// �� �鿡 ���� ����� �ʿ� �ִ� ���� ã�´�
		dx = elem.beam.begC.x - infoMorph.leftBottomX;
		dy = elem.beam.begC.y - infoMorph.leftBottomY;
		ang1 = RadToDegree (atan2 (dy, dx));	// �� �������� �� ���ϴ��� ���� ����
		dx = elem.beam.endC.x - infoMorph.leftBottomX;
		dy = elem.beam.endC.y - infoMorph.leftBottomY;
		ang2 = RadToDegree (atan2 (dy, dx));	// �� ������ �� ���ϴ��� ���� ����

		if (abs (infoMorph.ang - ang1) < EPS) {
			// ���� LeftBottom ��ǥ
			xPosLB = elem.beam.begC.x - elem.beam.width/2 * cos(DegreeToRad (infoMorph.ang));
			yPosLB = elem.beam.begC.y - elem.beam.width/2 * sin(DegreeToRad (infoMorph.ang));
			zPosLB = elem.beam.level - elem.beam.height;

			// ���� RightTop ��ǥ
			xPosRT = elem.beam.begC.x + elem.beam.width/2 * cos(DegreeToRad (infoMorph.ang));
			yPosRT = elem.beam.begC.y + elem.beam.width/2 * sin(DegreeToRad (infoMorph.ang));
			zPosRT = elem.beam.level;
		} else {
			// ���� LeftBottom ��ǥ
			xPosLB = elem.beam.endC.x - elem.beam.width/2 * cos(DegreeToRad (infoMorph.ang));
			yPosLB = elem.beam.endC.y - elem.beam.width/2 * sin(DegreeToRad (infoMorph.ang));
			zPosLB = elem.beam.level - elem.beam.height;

			// ���� RightTop ��ǥ
			xPosRT = elem.beam.endC.x + elem.beam.width/2 * cos(DegreeToRad (infoMorph.ang));
			yPosRT = elem.beam.endC.y + elem.beam.width/2 * sin(DegreeToRad (infoMorph.ang));
			zPosRT = elem.beam.level;
		}
		//} else {
		//	p1 = elem.beam.begC;
		//	p2 = elem.beam.endC;
		//	p3.x = infoMorph.leftBottomX;
		//	p3.y = infoMorph.leftBottomY;
		//	p4.x = infoMorph.rightTopX;
		//	p4.y = infoMorph.rightTopY;
		//	pi = IntersectionPoint1 (&p1, &p2, &p3, &p4);

		//	// ���� LeftBottom ��ǥ
		//	xPosLB = pi.x - elem.beam.width/2 * cos(DegreeToRad (infoMorph.ang));
		//	yPosLB = pi.y - elem.beam.width/2 * sin(DegreeToRad (infoMorph.ang));
		//	zPosLB = elem.beam.level - elem.beam.height;

		//	// ���� RightTop ��ǥ
		//	xPosRT = pi.x + elem.beam.width/2 * cos(DegreeToRad (infoMorph.ang));
		//	yPosRT = pi.y + elem.beam.width/2 * sin(DegreeToRad (infoMorph.ang));
		//	zPosRT = elem.beam.level;
		//}

		for (yy = 0 ; yy < 2 ; ++yy) {
			// X�� ���� ��
			result_compare_x = compareRanges (infoMorph.leftBottomX, infoMorph.leftBottomX + infoMorph.horLen * cos(DegreeToRad (infoMorph.ang)), xPosLB, xPosRT);

			// Y�� ���� ��
			result_compare_y = compareRanges (infoMorph.leftBottomY, infoMorph.leftBottomY + infoMorph.horLen * sin(DegreeToRad (infoMorph.ang)), yPosLB, yPosRT);

			// Z�� ���� ��
			result_compare_z = compareRanges (infoMorph.leftBottomZ, infoMorph.leftBottomZ + infoMorph.verLen, zPosLB, zPosRT);

			// �� ���� ��ܿ� �پ� ������ ó���ؾ� �� ����� �ν��� ��
			if ( (result_compare_x == 5) && (result_compare_y == 5) && (result_compare_z == 6) ) {
				placingZone.beams [xx].leftBottomX	= xPosLB;
				placingZone.beams [xx].leftBottomY	= yPosLB;
				placingZone.beams [xx].leftBottomZ	= zPosLB;
				placingZone.beams [xx].horLen		= elem.beam.width;
				placingZone.beams [xx].verLen		= elem.beam.height;
			}
		}

		ACAPI_DisposeElemMemoHdls (&memo);
	}

	// �۾� �� ���� �ݿ�
	BNZeroMemory (&storyInfo, sizeof (API_StoryInfo));
	minusLevel = 0.0;
	ACAPI_Environment (APIEnv_GetStorySettingsID, &storyInfo);
	for (xx = 0 ; xx < (storyInfo.lastStory - storyInfo.firstStory) ; ++xx) {
		if (storyInfo.data [0][xx].index == infoWall.floorInd) {
			minusLevel = storyInfo.data [0][xx].level;
			break;
		}
	}
	BMKillHandle ((GSHandle *) &storyInfo.data);

	// ��� ���� ������ Z ������ ����
	placingZone.leftBottomZ -= minusLevel;
	for (xx = 0 ; xx < placingZone.nInterfereBeams ; ++xx)	placingZone.beams [xx].leftBottomZ -= minusLevel;

	//////////////////////////////////////////////////////////// 1�� ������/���ڳ� ��ġ
	// [DIALOG] 1��° ���̾�α׿��� ���ڳ�, ������ ���� �Է� ����
	result = DGModalDialog (ACAPI_GetOwnResModule (), 32501, ACAPI_GetOwnResModule (), wallPlacerHandlerPrimary, 0);

	// ���ڿ��� �� �������� �ʺ�/���̸� �Ǽ������ε� ����
	placingZone.eu_wid_numeric = atof (placingZone.eu_wid.c_str ()) / 1000.0;
	placingZone.eu_hei_numeric = atof (placingZone.eu_hei.c_str ()) / 1000.0;

	// ���� ���� �ʱ�ȭ
	placingZone.remain_hor = placingZone.horLen;
	placingZone.remain_ver = placingZone.verLen;

	if (result != DG_OK)
		return err;

	// ���ڳ� ���� ���̸�ŭ ����
	if (placingZone.bLIncorner == true)		placingZone.remain_hor = placingZone.remain_hor - placingZone.lenLIncorner;
	if (placingZone.bRIncorner == true)		placingZone.remain_hor = placingZone.remain_hor - placingZone.lenRIncorner;

	// �������� ������ ���� �ʴ� ���� ���� ������Ʈ
	placingZone.remain_ver_wo_beams = placingZone.verLen;
	if (placingZone.nInterfereBeams > 0) {
		for (xx = 0 ; xx < placingZone.nInterfereBeams ; ++xx) {
			if (placingZone.remain_ver_wo_beams > placingZone.beams [xx].leftBottomZ)
				placingZone.remain_ver_wo_beams = placingZone.beams [xx].leftBottomZ;
		}
	}
	placingZone.remain_ver_wo_beams = placingZone.remain_ver_wo_beams;
	placingZone.remain_ver = placingZone.remain_ver_wo_beams;

	// ������ ����/���� ���� ���� ����
	placingZone.eu_count_hor = 0;
	placingZone.eu_count_ver = 0;

	if (placingZone.eu_ori.compare (std::string ("�������")) == 0) {
		placingZone.eu_count_hor = static_cast<short>(placingZone.remain_hor / placingZone.eu_wid_numeric);				// ���� ���� ����
		placingZone.remain_hor = placingZone.remain_hor - (placingZone.eu_count_hor * placingZone.eu_wid_numeric);		// ���� ���� ������
		placingZone.eu_count_ver = static_cast<short>(placingZone.remain_ver / placingZone.eu_hei_numeric);				// ���� ���� ����
		placingZone.remain_ver = placingZone.remain_ver - (placingZone.eu_count_ver * placingZone.eu_hei_numeric);		// ���� ���� ������
	} else {
		placingZone.eu_count_hor = static_cast<short>(placingZone.remain_hor / placingZone.eu_hei_numeric);				// ���� ���� ����
		placingZone.remain_hor = placingZone.remain_hor - (placingZone.eu_count_hor * placingZone.eu_hei_numeric);		// ���� ���� ������
		placingZone.eu_count_ver = static_cast<short>(placingZone.remain_ver / placingZone.eu_wid_numeric);				// ���� ���� ����
		placingZone.remain_ver = placingZone.remain_ver - (placingZone.eu_count_ver * placingZone.eu_wid_numeric);		// ���� ���� ������
	}

	// ���� ������ ���� �� �и� (������, ������)
	placingZone.remain_hor_updated = placingZone.remain_hor;

	// ���õ� ���� ���� ����
	err = ACAPI_Selection_Get (&selectionInfo, &selNeigs, true);
	BMKillHandle ((GSHandle *) &selectionInfo.marquee.coords);
	if (err != NoError) {
		BMKillHandle ((GSHandle *) &selNeigs);
		return err;
	}

	err = ACAPI_CallUndoableCommand ("���� ���� ����", [&] () -> GSErrCode {
		if (selectionInfo.typeID != API_SelEmpty) {
			nSel = BMGetHandleSize ((GSHandle) selNeigs) / sizeof (API_Neig);
			for (xx = 0 ; xx < nSel && err == NoError ; ++xx) {
				tElem.header.typeID = Neig_To_ElemID ((*selNeigs)[xx].neigID);

				if (tElem.header.typeID == API_MorphID) {	// ���� ����
					API_Elem_Head* headList = new API_Elem_Head [1];
					headList [0] = tElem.header;
					err = ACAPI_Element_Delete (&headList, 1);
					delete headList;
				}
			}
		}
		BMKillHandle ((GSHandle *) &selNeigs);

		return err;
	});

	// placingZone�� Cell ���� �ʱ�ȭ
	placingZone.nCells = (placingZone.eu_count_hor * 2) + 3;
	initCellsForWall (&placingZone);
	
	// �ݴ��� ���� ���� ���� ���� ���� �ʱ�ȭ
	initCellsForWall (&placingZoneBackside);
	placingZoneBackside.nCells = placingZone.nCells;

	// ��ġ�� ���� ���� �Է�
	firstPlacingSettingsForWall (&placingZone);
	copyPlacingZoneSymmetricForWall (&placingZone, &placingZoneBackside, &infoWall);

	// [DIALOG] 2��° ���̾�α׿��� ������/���ڳ� ��ġ�� �����ϰų� �ٷ������̼��� �����մϴ�.
	result = DGBlankModalDialog (185, 250, DG_DLG_VGROW | DG_DLG_HGROW, 0, DG_DLG_THICKFRAME, wallPlacerHandlerSecondary, 0);

	// ������ ���� ä���
	err = fillRestAreasForWall ();

	// [DIALOG] 4��° ���̾�α׿��� ä���� ������ ���� �߿��� ���� ������ �ٸ� �԰��� ���������� ��ü�� ������ �����ϴ�.
	//result = DGModalDialog (ACAPI_GetOwnResModule (), 32504, ACAPI_GetOwnResModule (), wallPlacerHandlerFourth, 0);
	
 // * �� �ֺ� ����/����:
 //   - ä��� ���� �ִ��� ��/�������� �⺻ ������ ä������
 //   - �׷��� �� �Ʒ��� �¿����鿡 ���� ������
 //      200*600 �̻��̸� ����ڿ��� ����
 // * ��� ������: ���� ������ ä��
 //   (1�� �Ǵ� 2������ ��? ������ ����(����/����)�� 50�̳� 0�̳Ŀ� ����
 //    ���簡 ���� �ٰų� ������ Ƣ��� �� ����

	return	err;
}

// ���� ä������ �Ϸ�� �� ������ ���� ä���
GSErrCode	fillRestAreasForWall (void)
{
	GSErrCode	err = NoError;

	short	xx, yy, zz;
	short	indInterfereBeam;		// ��ø�Ǵ� ���� �ε��� (-1�� ��ø ����)

	double	cellLeftX, cellRightX;	// ���� L/R�� X ��ǥ
	double	beamLeftX, beamRightX;	// ���� L/R�� X ��ǥ
	double	dist;

	Cell	insCell, insCellB;		// ������ �ӽ� ��
	double	insertedHeight;			// ȸ����Ų ������(����) �������� ���� �߰��� ����
	double	insertedLeft;			// ȸ����Ų ������(����) �������� ���� �߰��� �Ÿ�
	double	insertedRight;			// ȸ����Ų ������(����) �������� ���� �߰��� �Ÿ�

	err = ACAPI_CallUndoableCommand ("������ ���� ä���", [&] () -> GSErrCode {

		for (xx = placingZone.eu_count_ver ; xx < placingZone.eu_count_ver + 3 ; ++xx) {
			for (yy = 0 ; yy < placingZone.nCells ; ++yy) {

				insertedHeight = 0.0;
				insertedLeft = 0.0;
				insertedRight = 0.0;

				// ���� �������� ���� ���̰� �ƿ� ������ ���� ����
				if ( (placingZone.verLen - (placingZone.cells [xx-1][yy].leftBottomZ + placingZone.cells [xx-1][yy].verLen)) < 0)
					break;

				// ���� ��ø ���� Ȯ�� - ��ø�Ǵ� ���� �ε����� ���� ����
				indInterfereBeam = -1;
				for (zz = 0 ; zz < placingZone.nInterfereBeams ; ++zz) {

					// �����ϴ� ���� �ε��� ��ȣ�� ����
					if (indInterfereBeam == -1) {
						if (placingZone.cells [xx-1][yy].leftBottomZ + placingZone.cells [xx-1][yy].verLen * 2 >= placingZone.beams [zz].leftBottomZ) {
							dist = GetDistance (placingZone.leftBottomX, placingZone.leftBottomY, placingZone.cells [xx-1][yy].leftBottomX, placingZone.cells [xx-1][yy].leftBottomY);

							cellLeftX	= dist;
							cellRightX	= dist + placingZone.cells [xx-1][yy].horLen;
							dist		= GetDistance (placingZone.leftBottomX, placingZone.leftBottomY, placingZone.beams [zz].leftBottomX, placingZone.beams [zz].leftBottomY);
							beamLeftX	= dist;
							beamRightX	= dist + placingZone.beams [zz].horLen;

							// ���� ���� ���������� ħ���� ���
							if ( (cellLeftX < beamLeftX) && (beamLeftX < cellRightX) && (cellRightX <= beamRightX) )
								indInterfereBeam = zz;

							// ���� ���� �������� ħ���� ���
							if ( (cellLeftX < beamRightX) && (beamRightX < cellRightX) && (beamLeftX <= cellLeftX) )
								indInterfereBeam = zz;

							// ���� �� �ȿ� ������ ���
							if ( (cellLeftX < beamLeftX) && (beamRightX < cellRightX) )
								indInterfereBeam = zz;

							// ���� �� ������ �� ħ���� ���
							if ( (beamLeftX <= cellLeftX) && (cellRightX <= beamRightX) )
								indInterfereBeam = zz;
						}
					}
				}

				// ������ ������
				if (indInterfereBeam == -1) {

					// ���� ���̿��� �Ʒ� ��ü���� �÷��� �Ǵ� �����ΰ�?
					if ( (placingZone.verLen >= (placingZone.cells [xx-1][yy].leftBottomZ + placingZone.cells [xx-1][yy].verLen * 2)) || (abs (placingZone.verLen - (placingZone.cells [xx-1][yy].leftBottomZ + placingZone.cells [xx-1][yy].verLen * 2)) < EPS) ) {
						placingZone.cells [xx][yy] = placingZone.cells [xx-1][yy];
						placingZone.cells [xx][yy].leftBottomZ = placingZone.cells [xx-1][yy].leftBottomZ + placingZone.cells [xx-1][yy].verLen;
						placingZone.cells [xx][yy].guid = placeLibPart (placingZone.cells [xx][yy]);

						placingZoneBackside.cells [xx][yy] = placingZoneBackside.cells [xx-1][yy];
						placingZoneBackside.cells [xx][yy].leftBottomZ = placingZoneBackside.cells [xx-1][yy].leftBottomZ + placingZoneBackside.cells [xx-1][yy].verLen;
						placingZoneBackside.cells [xx][yy].guid = placeLibPart (placingZoneBackside.cells [xx][yy]);
					
					// ���̰� �����ϸ�
					} else if ( (placingZone.verLen - (placingZone.cells [xx-1][yy].leftBottomZ + placingZone.cells [xx-1][yy].verLen)) >= 0.010) {

						placingZone.cells [xx][yy] = placingZone.cells [xx-1][yy];
						placingZone.cells [xx][yy].leftBottomZ = placingZone.cells [xx-1][yy].leftBottomZ + placingZone.cells [xx-1][yy].verLen;

						placingZoneBackside.cells [xx][yy] = placingZoneBackside.cells [xx-1][yy];
						placingZoneBackside.cells [xx][yy].leftBottomZ = placingZoneBackside.cells [xx-1][yy].leftBottomZ + placingZoneBackside.cells [xx-1][yy].verLen;

						// ���ڳ�, ���ڳʿ� ������ �ٷ������̼��� ������ �ø�
						if ( ((placingZone.cells [xx][yy].objType == INCORNER) && ( (yy == 0) ) || (yy == (placingZone.nCells - 1 ))) || ((placingZone.cells [xx][yy].objType == FILLERSPACER) && ( (yy == 1) || (yy == (placingZone.nCells - 2)) )) ) {
						
							if (placingZone.cells [xx][yy].objType == INCORNER) {
								placingZone.cells [xx][yy].libPart.incorner.hei_s = placingZone.verLen - placingZone.cells [xx][yy].leftBottomZ;
								placingZoneBackside.cells [xx][yy].libPart.incorner.hei_s = placingZoneBackside.verLen - placingZoneBackside.cells [xx][yy].leftBottomZ;
							}
							if (placingZone.cells [xx][yy].objType == FILLERSPACER) {
								placingZone.cells [xx][yy].libPart.fillersp.f_leng = placingZone.verLen - placingZone.cells [xx][yy].leftBottomZ;
								placingZoneBackside.cells [xx][yy].libPart.fillersp.f_leng = placingZoneBackside.verLen - placingZoneBackside.cells [xx][yy].leftBottomZ;
							}

						// �� ���� ��ü�̸�, ����/�������� ä���
						} else {
							if ( ((placingZone.verLen - placingZone.cells [xx][yy].leftBottomZ) < 0.110) || (placingZone.cells [xx][yy].horLen < 0.110) ) {

								placingZone.cells [xx][yy].objType = WOOD;
								placingZone.cells [xx][yy].libPart.wood.w_w = 0.080;				// �β�: 80mm
								placingZone.cells [xx][yy].libPart.wood.w_leng = placingZone.cells [xx][yy].horLen;
								placingZone.cells [xx][yy].libPart.wood.w_h = placingZone.verLen - placingZone.cells [xx][yy].leftBottomZ;
								placingZone.cells [xx][yy].libPart.wood.w_ang = 0;

								placingZoneBackside.cells [xx][yy].objType = WOOD;
								placingZoneBackside.cells [xx][yy].libPart.wood.w_w = 0.080;		// �β�: 80mm
								placingZoneBackside.cells [xx][yy].libPart.wood.w_leng = placingZoneBackside.cells [xx][yy].horLen;
								placingZoneBackside.cells [xx][yy].libPart.wood.w_h = placingZoneBackside.verLen - placingZoneBackside.cells [xx][yy].leftBottomZ;
								placingZoneBackside.cells [xx][yy].libPart.wood.w_ang = 0;

							} else {

								placingZone.cells [xx][yy].objType = PLYWOOD;
								placingZone.cells [xx][yy].libPart.plywood.p_wid = placingZone.cells [xx][yy].horLen;
								placingZone.cells [xx][yy].libPart.plywood.p_leng = placingZone.verLen - placingZone.cells [xx][yy].leftBottomZ;
								placingZone.cells [xx][yy].libPart.plywood.w_dir_wall = true;

								placingZoneBackside.cells [xx][yy].objType = PLYWOOD;
								placingZoneBackside.cells [xx][yy].libPart.plywood.p_wid = placingZoneBackside.cells [xx][yy].horLen;
								placingZoneBackside.cells [xx][yy].libPart.plywood.p_leng = placingZone.verLen - placingZone.cells [xx][yy].leftBottomZ;
								placingZoneBackside.cells [xx][yy].libPart.plywood.w_dir_wall = true;
							}
						}
						
						placingZone.cells [xx][yy].horLen = placingZone.cells [xx-1][yy].horLen;
						placingZone.cells [xx][yy].verLen = placingZone.verLen - placingZone.cells [xx][yy].leftBottomZ;
						placingZone.cells [xx][yy].guid = placeLibPart (placingZone.cells [xx][yy]);

						placingZoneBackside.cells [xx][yy].horLen = placingZoneBackside.cells [xx-1][yy].horLen;
						placingZoneBackside.cells [xx][yy].verLen = placingZoneBackside.verLen - placingZoneBackside.cells [xx][yy].leftBottomZ;
						placingZoneBackside.cells [xx][yy].guid = placeLibPart (placingZoneBackside.cells [xx][yy]);
					}

				// ������ ������, �� �ֺ��� �����̳� ����� ä��
				} else {
					if ((placingZone.cells [xx-1][yy].leftBottomZ + placingZone.cells [xx-1][yy].verLen) <= placingZone.beams [indInterfereBeam].leftBottomZ) {
						// ���� ���� ���������� ħ���� ���
						if ( (cellLeftX < beamLeftX) && (beamLeftX < cellRightX) && (cellRightX <= beamRightX) ) {
							// ���� ���� Ÿ���� ������(�������)�� ��쿡 ���� ȸ������ ��ġ �õ�
							if ( (placingZone.cells [xx-1][yy].objType == EUROFORM) && (placingZone.cells [xx-1][yy].libPart.form.u_ins_wall == true) ) {
								// �������� ������ �� ������ �Ǹ�,
								if ( (placingZone.cells [xx-1][yy].leftBottomZ + placingZone.cells [xx-1][yy].verLen + placingZone.cells [xx-1][yy].horLen) <= placingZone.beams [indInterfereBeam].leftBottomZ) {
									// ������ ������ ��ġ
									insCell = placingZone.cells [xx-1][yy];
									insCell.objType = EUROFORM;
									insCell.leftBottomX = placingZone.cells [xx-1][yy].leftBottomX + (placingZone.cells [xx-1][yy].horLen * cos(placingZone.cells [xx-1][yy].ang));
									insCell.leftBottomY = placingZone.cells [xx-1][yy].leftBottomY + (placingZone.cells [xx-1][yy].horLen * sin(placingZone.cells [xx-1][yy].ang));
									insCell.leftBottomZ = placingZone.cells [xx-1][yy].leftBottomZ + placingZone.cells [xx-1][yy].verLen;
									insCell.ang = placingZone.cells [xx-1][yy].ang;
									insCell.libPart.form.u_ins_wall = false;

									insCellB = placingZoneBackside.cells [xx-1][yy];
									insCellB.objType = EUROFORM;
									insCellB.leftBottomX = placingZoneBackside.cells [xx-1][yy].leftBottomX;
									insCellB.leftBottomY = placingZoneBackside.cells [xx-1][yy].leftBottomY;
									insCellB.leftBottomZ = placingZoneBackside.cells [xx-1][yy].leftBottomZ + placingZoneBackside.cells [xx-1][yy].verLen;
									insCellB.ang = placingZoneBackside.cells [xx-1][yy].ang;
									insCellB.libPart.form.u_ins_wall = false;

									placeLibPart (insCell);
									placeLibPart (insCellB);

									if (insCell.libPart.form.eu_stan_onoff == true)
										insertedHeight += insCell.libPart.form.eu_wid;
									else
										insertedHeight += insCell.libPart.form.eu_wid2;
								}
							}
							
							// ���� ���� Ÿ���� ������(��������)�� ��쿡 ���� ȸ������ ��ġ �õ�
							if ( (placingZone.cells [xx-1][yy].objType == EUROFORM) && (placingZone.cells [xx-1][yy].libPart.form.u_ins_wall == false) ) {
								// �������� ������ �� ������ �Ǹ�,
								if ( (placingZone.verLen >= (placingZone.cells [xx-1][yy].leftBottomZ + placingZone.cells [xx-1][yy].verLen + placingZone.cells [xx-1][yy].horLen)) ) {
									// �� ������ �ʺ� ������ ª���� ���� �̻��̸�,
									if ( (beamRightX - cellLeftX) >= placingZone.cells [xx-1][yy].verLen ) {
										
										// �������� ������ ��ġ
										insCell = placingZone.cells [xx-1][yy];
										insCell.objType = EUROFORM;
										insCell.leftBottomX = placingZone.cells [xx-1][yy].leftBottomX;
										insCell.leftBottomY = placingZone.cells [xx-1][yy].leftBottomY;
										insCell.leftBottomZ = placingZone.cells [xx-1][yy].leftBottomZ + placingZone.cells [xx-1][yy].verLen;
										insCell.ang = placingZone.cells [xx-1][yy].ang;
										insCell.libPart.form.u_ins_wall = true;

										insCellB = placingZoneBackside.cells [xx-1][yy];
										insCellB.objType = EUROFORM;
										insCellB.leftBottomX = placingZoneBackside.cells [xx-1][yy].leftBottomX + (placingZoneBackside.cells [xx-1][yy].verLen * cos(placingZoneBackside.cells [xx-1][yy].ang));
										insCellB.leftBottomY = placingZoneBackside.cells [xx-1][yy].leftBottomY + (placingZoneBackside.cells [xx-1][yy].verLen * sin(placingZoneBackside.cells [xx-1][yy].ang));
										insCellB.leftBottomZ = placingZoneBackside.cells [xx-1][yy].leftBottomZ + placingZoneBackside.cells [xx-1][yy].verLen;
										insCellB.ang = placingZoneBackside.cells [xx-1][yy].ang;
										insCellB.libPart.form.u_ins_wall = true;

										placeLibPart (insCell);
										placeLibPart (insCellB);

										// ���� ����/���� �߰�
										insCell.leftBottomX = placingZone.cells [xx-1][yy].leftBottomX;
										insCell.leftBottomY = placingZone.cells [xx-1][yy].leftBottomY;
										insCell.leftBottomZ = placingZone.cells [xx-1][yy].leftBottomZ + placingZone.cells [xx-1][yy].verLen + placingZone.cells [xx-1][yy].horLen;
										insCell.ang = placingZone.cells [xx-1][yy].ang;

										insCellB.leftBottomX = placingZoneBackside.cells [xx-1][yy].leftBottomX + (placingZoneBackside.cells [xx-1][yy].verLen * cos(placingZoneBackside.cells [xx-1][yy].ang));
										insCellB.leftBottomY = placingZoneBackside.cells [xx-1][yy].leftBottomY + (placingZoneBackside.cells [xx-1][yy].verLen * sin(placingZoneBackside.cells [xx-1][yy].ang));
										insCellB.leftBottomZ = placingZoneBackside.cells [xx-1][yy].leftBottomZ + placingZoneBackside.cells [xx-1][yy].verLen + placingZoneBackside.cells [xx-1][yy].horLen;
										insCellB.ang = placingZoneBackside.cells [xx-1][yy].ang;

										if ((placingZone.verLen - (placingZone.cells [xx-1][yy].leftBottomZ + placingZone.cells [xx-1][yy].verLen + placingZone.cells [xx-1][yy].horLen)) < 0.110) {
											// ���� 110mm �̸��̸� ����
											insCell.objType = WOOD;
											insCell.libPart.wood.w_w = 0.080;		// �β�: 80mm
											insCell.libPart.wood.w_leng = placingZone.verLen - (placingZone.cells [xx-1][yy].leftBottomZ + placingZone.cells [xx-1][yy].verLen + placingZone.cells [xx-1][yy].horLen);
											insCell.libPart.wood.w_h = placingZone.cells [xx-1][yy].verLen;
											insCell.libPart.wood.w_ang = DegreeToRad (90.0);

											insCellB.objType = WOOD;
											insCellB.libPart.wood.w_w = 0.080;		// �β�: 80mm
											insCellB.libPart.wood.w_leng = placingZone.verLen - (placingZone.cells [xx-1][yy].leftBottomZ + placingZone.cells [xx-1][yy].verLen + placingZone.cells [xx-1][yy].horLen);
											insCellB.libPart.wood.w_h = placingZone.cells [xx-1][yy].verLen;
											insCellB.libPart.wood.w_ang = DegreeToRad (90.0);
										} else {
											// ���� 110mm �̻��̸� ����
											insCell.objType = PLYWOOD;
											insCell.libPart.plywood.p_wid = placingZone.cells [xx-1][yy].verLen;
											insCell.libPart.plywood.p_leng = placingZone.verLen - (placingZone.cells [xx-1][yy].leftBottomZ + placingZone.cells [xx-1][yy].verLen + placingZone.cells [xx-1][yy].horLen);
											insCell.libPart.plywood.w_dir_wall = true;

											insCellB.objType = PLYWOOD;
											insCellB.libPart.plywood.p_wid = placingZone.cells [xx-1][yy].verLen;
											insCellB.libPart.plywood.p_leng = placingZone.verLen - (placingZone.cells [xx-1][yy].leftBottomZ + placingZone.cells [xx-1][yy].verLen + placingZone.cells [xx-1][yy].horLen);
											insCellB.libPart.plywood.w_dir_wall = true;
										}
										placeLibPart (insCell);
										placeLibPart (insCellB);

										// ��ǥ�� ����
										insertedLeft = placingZone.cells [xx-1][yy].verLen;
									}
								}
							}

							// �� ������
							insCell.leftBottomX = placingZone.cells [xx-1][yy].leftBottomX + insertedLeft * cos(placingZone.cells [xx-1][yy].ang);
							insCell.leftBottomY = placingZone.cells [xx-1][yy].leftBottomY + insertedLeft * sin(placingZone.cells [xx-1][yy].ang);
							insCell.leftBottomZ = placingZone.cells [xx-1][yy].leftBottomZ + placingZone.cells [xx-1][yy].verLen + insertedHeight;
							insCell.ang = placingZone.cells [xx-1][yy].ang;

							insCellB.leftBottomX = placingZoneBackside.cells [xx-1][yy].leftBottomX + ((placingZoneBackside.cells [xx-1][yy].horLen - (beamLeftX - cellLeftX)) * cos(placingZoneBackside.cells [xx-1][yy].ang));
							insCellB.leftBottomY = placingZoneBackside.cells [xx-1][yy].leftBottomY + ((placingZoneBackside.cells [xx-1][yy].horLen - (beamLeftX - cellLeftX)) * sin(placingZoneBackside.cells [xx-1][yy].ang));
							insCellB.leftBottomZ = placingZoneBackside.cells [xx-1][yy].leftBottomZ + placingZoneBackside.cells [xx-1][yy].verLen + insertedHeight;
							insCellB.ang = placingZoneBackside.cells [xx-1][yy].ang;

							if ((beamLeftX - cellLeftX) < 0.110) {
								// ���� 110mm �̸��̸� ����
								insCell.objType = WOOD;
								insCell.libPart.wood.w_w = 0.080;				// �β�: 80mm
								insCell.libPart.wood.w_leng = placingZone.verLen - (placingZone.cells [xx-1][yy].leftBottomZ + placingZone.cells [xx-1][yy].verLen + insertedHeight);
								insCell.libPart.wood.w_h = beamLeftX - cellLeftX - insertedLeft;
								insCell.libPart.wood.w_ang = DegreeToRad (90.0);

								insCellB.objType = WOOD;
								insCellB.libPart.wood.w_w = 0.080;		// �β�: 80mm
								insCellB.libPart.wood.w_leng = placingZoneBackside.verLen - (placingZoneBackside.cells [xx-1][yy].leftBottomZ + placingZoneBackside.cells [xx-1][yy].verLen + insertedHeight);
								insCellB.libPart.wood.w_h = beamLeftX - cellLeftX - insertedLeft;
								insCellB.libPart.wood.w_ang = DegreeToRad (90.0);
							} else {
								// ���� 110mm �̻��̸� ����
								insCell.objType = PLYWOOD;
								insCell.libPart.plywood.p_wid = beamLeftX - cellLeftX - insertedLeft;
								insCell.libPart.plywood.p_leng = placingZone.verLen - (placingZone.cells [xx-1][yy].leftBottomZ + placingZone.cells [xx-1][yy].verLen + insertedHeight);
								insCell.libPart.plywood.w_dir_wall = true;

								insCellB.objType = PLYWOOD;
								insCellB.libPart.plywood.p_wid = beamLeftX - cellLeftX - insertedLeft;
								insCellB.libPart.plywood.p_leng = placingZoneBackside.verLen - (placingZoneBackside.cells [xx-1][yy].leftBottomZ + placingZoneBackside.cells [xx-1][yy].verLen + insertedHeight);
								insCellB.libPart.plywood.w_dir_wall = true;
							}
							placeLibPart (insCell);
							placeLibPart (insCellB);

							// �� �Ʒ���
							insCell.leftBottomX = placingZone.cells [xx-1][yy].leftBottomX - ((beamLeftX - cellLeftX) * cos(placingZoneBackside.cells [xx-1][yy].ang));
							insCell.leftBottomY = placingZone.cells [xx-1][yy].leftBottomY - ((beamLeftX - cellLeftX) * sin(placingZoneBackside.cells [xx-1][yy].ang));
							insCell.leftBottomZ = placingZone.cells [xx-1][yy].leftBottomZ + placingZone.cells [xx-1][yy].verLen + insertedHeight;
							insCell.ang = placingZone.cells [xx-1][yy].ang;

							insCellB.leftBottomX = placingZoneBackside.cells [xx-1][yy].leftBottomX + ((cellRightX - beamRightX) * cos(placingZoneBackside.cells [xx-1][yy].ang));
							insCellB.leftBottomY = placingZoneBackside.cells [xx-1][yy].leftBottomY + ((cellRightX - beamRightX) * sin(placingZoneBackside.cells [xx-1][yy].ang));
							insCellB.leftBottomZ = placingZoneBackside.cells [xx-1][yy].leftBottomZ + placingZoneBackside.cells [xx-1][yy].verLen + insertedHeight;
							insCellB.ang = placingZoneBackside.cells [xx-1][yy].ang;

							if ((placingZone.beams [indInterfereBeam].leftBottomZ - (placingZoneBackside.cells [xx-1][yy].leftBottomZ + placingZoneBackside.cells [xx-1][yy].verLen)) < 0.110) {
								// ���� 110mm �̸��̸� ����
								insCell.objType = WOOD;
								insCell.libPart.wood.w_w = 0.080;				// �β�: 80mm
								insCell.libPart.wood.w_leng = placingZone.beams [indInterfereBeam].leftBottomZ - (placingZoneBackside.cells [xx-1][yy].leftBottomZ + placingZoneBackside.cells [xx-1][yy].verLen + insertedHeight);
								insCell.libPart.wood.w_h = beamRightX - beamLeftX;
								insCell.libPart.wood.w_ang = DegreeToRad (90.0);

								insCellB.objType = WOOD;
								insCellB.libPart.wood.w_w = 0.080;		// �β�: 80mm
								insCellB.libPart.wood.w_leng = placingZone.beams [indInterfereBeam].leftBottomZ - (placingZoneBackside.cells [xx-1][yy].leftBottomZ + placingZoneBackside.cells [xx-1][yy].verLen + insertedHeight);
								insCellB.libPart.wood.w_h = beamRightX - beamLeftX;
								insCellB.libPart.wood.w_ang = DegreeToRad (90.0);
							} else {
								// ���� 110mm �̻��̸� ����
								insCell.objType = PLYWOOD;
								insCell.libPart.plywood.p_wid = beamRightX - beamLeftX;
								insCell.libPart.plywood.p_leng = placingZone.beams [indInterfereBeam].leftBottomZ - (placingZoneBackside.cells [xx-1][yy].leftBottomZ + placingZoneBackside.cells [xx-1][yy].verLen + insertedHeight);
								insCell.libPart.plywood.w_dir_wall = true;

								insCellB.objType = PLYWOOD;
								insCellB.libPart.plywood.p_wid = beamRightX - beamLeftX;
								insCellB.libPart.plywood.p_leng = placingZone.beams [indInterfereBeam].leftBottomZ - (placingZoneBackside.cells [xx-1][yy].leftBottomZ + placingZoneBackside.cells [xx-1][yy].verLen + insertedHeight);
								insCellB.libPart.plywood.w_dir_wall = true;
							}
							placeLibPart (insCell);
							placeLibPart (insCellB);
						}

						// ���� ���� �������� ħ���� ���
						if ( (cellLeftX < beamRightX) && (beamRightX < cellRightX) && (beamLeftX <= cellLeftX) ) {
							// ���� ���� Ÿ���� ������(�������)�� ���
							if ( (placingZone.cells [xx-1][yy].objType == EUROFORM) && (placingZone.cells [xx-1][yy].libPart.form.u_ins_wall == true) ) {
								// �������� ������ �� ������ �Ǹ�,
								if ( (placingZone.cells [xx-1][yy].leftBottomZ + placingZone.cells [xx-1][yy].verLen + placingZone.cells [xx-1][yy].horLen) <= placingZone.beams [indInterfereBeam].leftBottomZ) {
									// ������ ������ ��ġ�Ǵ� ���� ����Ͽ� �� ���� Ȯ��
									insertedHeight += placingZone.cells [xx-1][yy].horLen;
								}
							}

							// ���� ���� Ÿ���� ������(��������)�� ��쿡 ���� ȸ������ ��ġ �õ�
							if ( (placingZone.cells [xx-1][yy].objType == EUROFORM) && (placingZone.cells [xx-1][yy].libPart.form.u_ins_wall == false) ) {
								// �������� ������ �� ������ �Ǹ�,
								if ( (placingZone.verLen >= (placingZone.cells [xx-1][yy].leftBottomZ + placingZone.cells [xx-1][yy].verLen + placingZone.cells [xx-1][yy].horLen)) ) {
									// �� ������ �ʺ� ������ ª���� ���� �̻��̸�,
									if ( (cellRightX - beamRightX) >= placingZone.cells [xx-1][yy].verLen ) {
										
										// �������� ������ ��ġ
										insCell = placingZone.cells [xx-1][yy];
										insCell.objType = EUROFORM;
										insCell.leftBottomX = placingZone.cells [xx-1][yy].leftBottomX + (placingZone.cells [xx-1][yy].verLen * cos(placingZone.cells [xx-1][yy].ang));
										insCell.leftBottomY = placingZone.cells [xx-1][yy].leftBottomY + (placingZone.cells [xx-1][yy].verLen * sin(placingZone.cells [xx-1][yy].ang));
										insCell.leftBottomZ = placingZone.cells [xx-1][yy].leftBottomZ + placingZone.cells [xx-1][yy].verLen;
										insCell.ang = placingZone.cells [xx-1][yy].ang;
										insCell.libPart.form.u_ins_wall = true;

										insCellB = placingZoneBackside.cells [xx-1][yy];
										insCellB.objType = EUROFORM;
										insCellB.leftBottomX = placingZoneBackside.cells [xx-1][yy].leftBottomX;
										insCellB.leftBottomY = placingZoneBackside.cells [xx-1][yy].leftBottomY;
										insCellB.leftBottomZ = placingZoneBackside.cells [xx-1][yy].leftBottomZ + placingZoneBackside.cells [xx-1][yy].verLen;
										insCellB.ang = placingZoneBackside.cells [xx-1][yy].ang;
										insCellB.libPart.form.u_ins_wall = true;

										placeLibPart (insCell);
										placeLibPart (insCellB);

										// ���� ����/���� �߰�
										insCell.leftBottomX = placingZone.cells [xx-1][yy].leftBottomX + (placingZone.cells [xx-1][yy].verLen * cos(placingZone.cells [xx-1][yy].ang));
										insCell.leftBottomY = placingZone.cells [xx-1][yy].leftBottomY + (placingZone.cells [xx-1][yy].verLen * sin(placingZone.cells [xx-1][yy].ang));
										insCell.leftBottomZ = placingZone.cells [xx-1][yy].leftBottomZ + placingZone.cells [xx-1][yy].verLen + placingZone.cells [xx-1][yy].horLen;
										insCell.ang = placingZone.cells [xx-1][yy].ang;

										insCellB.leftBottomX = placingZoneBackside.cells [xx-1][yy].leftBottomX;
										insCellB.leftBottomY = placingZoneBackside.cells [xx-1][yy].leftBottomY;
										insCellB.leftBottomZ = placingZoneBackside.cells [xx-1][yy].leftBottomZ + placingZoneBackside.cells [xx-1][yy].verLen + placingZoneBackside.cells [xx-1][yy].horLen;
										insCellB.ang = placingZoneBackside.cells [xx-1][yy].ang;

										if ((placingZone.verLen - (placingZone.cells [xx-1][yy].leftBottomZ + placingZone.cells [xx-1][yy].verLen + placingZone.cells [xx-1][yy].horLen)) < 0.110) {
											// ���� 110mm �̸��̸� ����
											insCell.objType = WOOD;
											insCell.libPart.wood.w_w = 0.080;				// �β�: 80mm
											insCell.libPart.wood.w_leng = placingZone.verLen - (placingZone.cells [xx-1][yy].leftBottomZ + placingZone.cells [xx-1][yy].verLen + placingZone.cells [xx-1][yy].horLen);
											insCell.libPart.wood.w_h = placingZone.cells [xx-1][yy].verLen;
											insCell.libPart.wood.w_ang = DegreeToRad (90.0);

											insCellB.objType = WOOD;
											insCellB.libPart.wood.w_w = 0.080;		// �β�: 80mm
											insCellB.libPart.wood.w_leng = placingZone.verLen - (placingZone.cells [xx-1][yy].leftBottomZ + placingZone.cells [xx-1][yy].verLen + placingZone.cells [xx-1][yy].horLen);
											insCellB.libPart.wood.w_h = placingZone.cells [xx-1][yy].verLen;
											insCellB.libPart.wood.w_ang = DegreeToRad (90.0);
										} else {
											// ���� 110mm �̻��̸� ����
											insCell.objType = PLYWOOD;
											insCell.libPart.plywood.p_wid = placingZone.cells [xx-1][yy].verLen;
											insCell.libPart.plywood.p_leng = placingZone.verLen - (placingZone.cells [xx-1][yy].leftBottomZ + placingZone.cells [xx-1][yy].verLen + placingZone.cells [xx-1][yy].horLen);
											insCell.libPart.plywood.w_dir_wall = true;

											insCellB.objType = PLYWOOD;
											insCellB.libPart.plywood.p_wid = placingZone.cells [xx-1][yy].verLen;
											insCellB.libPart.plywood.p_leng = placingZone.verLen - (placingZone.cells [xx-1][yy].leftBottomZ + placingZone.cells [xx-1][yy].verLen + placingZone.cells [xx-1][yy].horLen);
											insCellB.libPart.plywood.w_dir_wall = true;
										}
										placeLibPart (insCell);
										placeLibPart (insCellB);

										// ��ǥ�� ����
										insertedRight = placingZone.cells [xx-1][yy].verLen;
									}
								}
							}

							// �� ������
							insCell.leftBottomX = placingZone.cells [xx-1][yy].leftBottomX - ((cellLeftX - beamRightX) * cos(placingZone.cells [xx-1][yy].ang));
							insCell.leftBottomY = placingZone.cells [xx-1][yy].leftBottomY - ((cellLeftX - beamRightX) * sin(placingZone.cells [xx-1][yy].ang));
							insCell.leftBottomZ = placingZone.cells [xx-1][yy].leftBottomZ + placingZone.cells [xx-1][yy].verLen + insertedHeight;
							insCell.ang = placingZone.cells [xx-1][yy].ang;

							insCellB.leftBottomX = placingZoneBackside.cells [xx-1][yy].leftBottomX + (insertedRight * cos(placingZoneBackside.cells [xx-1][yy].ang));
							insCellB.leftBottomY = placingZoneBackside.cells [xx-1][yy].leftBottomY + (insertedRight * sin(placingZoneBackside.cells [xx-1][yy].ang));
							insCellB.leftBottomZ = placingZoneBackside.cells [xx-1][yy].leftBottomZ + placingZoneBackside.cells [xx-1][yy].verLen + insertedHeight;
							insCellB.ang = placingZoneBackside.cells [xx-1][yy].ang;

							if ((cellRightX - beamRightX) < 0.110) {
								// ���� 110mm �̸��̸� ����
								insCell.objType = WOOD;
								insCell.libPart.wood.w_w = 0.080;				// �β�: 80mm
								insCell.libPart.wood.w_leng = placingZone.verLen - (placingZone.cells [xx-1][yy].leftBottomZ + placingZone.cells [xx-1][yy].verLen + insertedHeight);
								insCell.libPart.wood.w_h = cellRightX - beamRightX - insertedRight;
								insCell.libPart.wood.w_ang = DegreeToRad (90.0);

								insCellB.objType = WOOD;
								insCellB.libPart.wood.w_w = 0.080;		// �β�: 80mm
								insCellB.libPart.wood.w_leng = placingZoneBackside.verLen - (placingZoneBackside.cells [xx-1][yy].leftBottomZ + placingZoneBackside.cells [xx-1][yy].verLen + insertedHeight);
								insCellB.libPart.wood.w_h = cellRightX - beamRightX - insertedRight;
								insCellB.libPart.wood.w_ang = DegreeToRad (90.0);
							} else {
								// ���� 110mm �̻��̸� ����
								insCell.objType = PLYWOOD;
								insCell.libPart.plywood.p_wid = cellRightX - beamRightX - insertedRight;
								insCell.libPart.plywood.p_leng = placingZone.verLen - (placingZone.cells [xx-1][yy].leftBottomZ + placingZone.cells [xx-1][yy].verLen + insertedHeight);
								insCell.libPart.plywood.w_dir_wall = true;

								insCellB.objType = PLYWOOD;
								insCellB.libPart.plywood.p_wid = cellRightX - beamRightX - insertedRight;
								insCellB.libPart.plywood.p_leng = placingZoneBackside.verLen - (placingZoneBackside.cells [xx-1][yy].leftBottomZ + placingZoneBackside.cells [xx-1][yy].verLen + insertedHeight);
								insCellB.libPart.plywood.w_dir_wall = true;
							}
							placeLibPart (insCell);
							placeLibPart (insCellB);
						}

						// ���� �� �ȿ� ������ ���
						if ( (cellLeftX < beamLeftX) && (beamRightX < cellRightX) ) {

							// �� ������
							insCell.leftBottomX = placingZone.cells [xx-1][yy].leftBottomX;
							insCell.leftBottomY = placingZone.cells [xx-1][yy].leftBottomY;
							insCell.leftBottomZ = placingZone.cells [xx-1][yy].leftBottomZ + placingZone.cells [xx-1][yy].verLen;
							insCell.ang = placingZone.cells [xx-1][yy].ang;

							insCellB.leftBottomX = placingZoneBackside.cells [xx-1][yy].leftBottomX + ((placingZoneBackside.cells [xx-1][yy].horLen - (beamLeftX - cellLeftX)) * cos(placingZoneBackside.cells [xx-1][yy].ang));
							insCellB.leftBottomY = placingZoneBackside.cells [xx-1][yy].leftBottomY + ((placingZoneBackside.cells [xx-1][yy].horLen - (beamLeftX - cellLeftX)) * sin(placingZoneBackside.cells [xx-1][yy].ang));
							insCellB.leftBottomZ = placingZoneBackside.cells [xx-1][yy].leftBottomZ + placingZoneBackside.cells [xx-1][yy].verLen;
							insCellB.ang = placingZoneBackside.cells [xx-1][yy].ang;

							if ((beamLeftX - cellLeftX) < 0.110) {
								// ���� 110mm �̸��̸� ����
								insCell.objType = WOOD;
								insCell.libPart.wood.w_w = 0.080;				// �β�: 80mm
								insCell.libPart.wood.w_leng = placingZone.verLen - (placingZone.cells [xx-1][yy].leftBottomZ + placingZone.cells [xx-1][yy].verLen);
								insCell.libPart.wood.w_h = beamLeftX - cellLeftX;
								insCell.libPart.wood.w_ang = DegreeToRad (90.0);

								insCellB.objType = WOOD;
								insCellB.libPart.wood.w_w = 0.080;		// �β�: 80mm
								insCellB.libPart.wood.w_leng = placingZoneBackside.verLen - (placingZoneBackside.cells [xx-1][yy].leftBottomZ + placingZoneBackside.cells [xx-1][yy].verLen);
								insCellB.libPart.wood.w_h = beamLeftX - cellLeftX;
								insCellB.libPart.wood.w_ang = DegreeToRad (90.0);
							} else {
								// ���� 110mm �̻��̸� ����
								insCell.objType = PLYWOOD;
								insCell.libPart.plywood.p_wid = beamLeftX - cellLeftX;
								insCell.libPart.plywood.p_leng = placingZone.verLen - (placingZone.cells [xx-1][yy].leftBottomZ + placingZone.cells [xx-1][yy].verLen);
								insCell.libPart.plywood.w_dir_wall = true;

								insCellB.objType = PLYWOOD;
								insCellB.libPart.plywood.p_wid = beamLeftX - cellLeftX;
								insCellB.libPart.plywood.p_leng = placingZoneBackside.verLen - (placingZoneBackside.cells [xx-1][yy].leftBottomZ + placingZoneBackside.cells [xx-1][yy].verLen);
								insCellB.libPart.plywood.w_dir_wall = true;
							}
							placeLibPart (insCell);
							placeLibPart (insCellB);

							// �� �Ʒ���
							insCell.leftBottomX = placingZone.cells [xx-1][yy].leftBottomX - ((beamLeftX - cellLeftX) * cos(placingZoneBackside.cells [xx-1][yy].ang));
							insCell.leftBottomY = placingZone.cells [xx-1][yy].leftBottomY - ((beamLeftX - cellLeftX) * sin(placingZoneBackside.cells [xx-1][yy].ang));
							insCell.leftBottomZ = placingZone.cells [xx-1][yy].leftBottomZ + placingZone.cells [xx-1][yy].verLen;
							insCell.ang = placingZone.cells [xx-1][yy].ang;

							insCellB.leftBottomX = placingZoneBackside.cells [xx-1][yy].leftBottomX + ((cellRightX - beamRightX) * cos(placingZoneBackside.cells [xx-1][yy].ang));
							insCellB.leftBottomY = placingZoneBackside.cells [xx-1][yy].leftBottomY + ((cellRightX - beamRightX) * sin(placingZoneBackside.cells [xx-1][yy].ang));
							insCellB.leftBottomZ = placingZoneBackside.cells [xx-1][yy].leftBottomZ + placingZoneBackside.cells [xx-1][yy].verLen;
							insCellB.ang = placingZoneBackside.cells [xx-1][yy].ang;

							if ((placingZone.beams [indInterfereBeam].leftBottomZ - (placingZoneBackside.cells [xx-1][yy].leftBottomZ + placingZoneBackside.cells [xx-1][yy].verLen)) < 0.110) {
								// ���� 110mm �̸��̸� ����
								insCell.objType = WOOD;
								insCell.libPart.wood.w_w = 0.080;				// �β�: 80mm
								insCell.libPart.wood.w_leng = placingZone.beams [indInterfereBeam].leftBottomZ - (placingZoneBackside.cells [xx-1][yy].leftBottomZ + placingZoneBackside.cells [xx-1][yy].verLen);
								insCell.libPart.wood.w_h = beamRightX - beamLeftX;
								insCell.libPart.wood.w_ang = DegreeToRad (90.0);

								insCellB.objType = WOOD;
								insCellB.libPart.wood.w_w = 0.080;		// �β�: 80mm
								insCellB.libPart.wood.w_leng = placingZone.beams [indInterfereBeam].leftBottomZ - (placingZoneBackside.cells [xx-1][yy].leftBottomZ + placingZoneBackside.cells [xx-1][yy].verLen);
								insCellB.libPart.wood.w_h = beamRightX - beamLeftX;
								insCellB.libPart.wood.w_ang = DegreeToRad (90.0);
							} else {
								// ���� 110mm �̻��̸� ����
								insCell.objType = PLYWOOD;
								insCell.libPart.plywood.p_wid = beamRightX - beamLeftX;
								insCell.libPart.plywood.p_leng = placingZone.beams [indInterfereBeam].leftBottomZ - (placingZoneBackside.cells [xx-1][yy].leftBottomZ + placingZoneBackside.cells [xx-1][yy].verLen);
								insCell.libPart.plywood.w_dir_wall = true;

								insCellB.objType = PLYWOOD;
								insCellB.libPart.plywood.p_wid = beamRightX - beamLeftX;
								insCellB.libPart.plywood.p_leng = placingZone.beams [indInterfereBeam].leftBottomZ - (placingZoneBackside.cells [xx-1][yy].leftBottomZ + placingZoneBackside.cells [xx-1][yy].verLen);
								insCellB.libPart.plywood.w_dir_wall = true;
							}
							placeLibPart (insCell);
							placeLibPart (insCellB);

							// �� ������
							insCell.leftBottomX = placingZone.cells [xx-1][yy].leftBottomX - ((beamRightX - cellLeftX) * cos(placingZoneBackside.cells [xx-1][yy].ang));
							insCell.leftBottomY = placingZone.cells [xx-1][yy].leftBottomY - ((beamRightX - cellLeftX) * sin(placingZoneBackside.cells [xx-1][yy].ang));
							insCell.leftBottomZ = placingZone.cells [xx-1][yy].leftBottomZ + placingZone.cells [xx-1][yy].verLen;
							insCell.ang = placingZone.cells [xx-1][yy].ang;

							insCellB.leftBottomX = placingZoneBackside.cells [xx-1][yy].leftBottomX;
							insCellB.leftBottomY = placingZoneBackside.cells [xx-1][yy].leftBottomY;
							insCellB.leftBottomZ = placingZoneBackside.cells [xx-1][yy].leftBottomZ + placingZoneBackside.cells [xx-1][yy].verLen;
							insCellB.ang = placingZoneBackside.cells [xx-1][yy].ang;

							if ((cellRightX - beamRightX) < 0.110) {
								// ���� 110mm �̸��̸� ����
								insCell.objType = WOOD;
								insCell.libPart.wood.w_w = 0.080;				// �β�: 80mm
								insCell.libPart.wood.w_leng = placingZone.verLen - (placingZone.cells [xx-1][yy].leftBottomZ + placingZone.cells [xx-1][yy].verLen);
								insCell.libPart.wood.w_h = cellRightX - beamRightX;
								insCell.libPart.wood.w_ang = DegreeToRad (90.0);

								insCellB.objType = WOOD;
								insCellB.libPart.wood.w_w = 0.080;		// �β�: 80mm
								insCellB.libPart.wood.w_leng = placingZoneBackside.verLen - (placingZoneBackside.cells [xx-1][yy].leftBottomZ + placingZoneBackside.cells [xx-1][yy].verLen);
								insCellB.libPart.wood.w_h = cellRightX - beamRightX;
								insCellB.libPart.wood.w_ang = DegreeToRad (90.0);
							} else {
								// ���� 110mm �̻��̸� ����
								insCell.objType = PLYWOOD;
								insCell.libPart.plywood.p_wid = cellRightX - beamRightX;
								insCell.libPart.plywood.p_leng = placingZone.verLen - (placingZone.cells [xx-1][yy].leftBottomZ + placingZone.cells [xx-1][yy].verLen);
								insCell.libPart.plywood.w_dir_wall = true;

								insCellB.objType = PLYWOOD;
								insCellB.libPart.plywood.p_wid = cellRightX - beamRightX;
								insCellB.libPart.plywood.p_leng = placingZoneBackside.verLen - (placingZoneBackside.cells [xx-1][yy].leftBottomZ + placingZoneBackside.cells [xx-1][yy].verLen);
								insCellB.libPart.plywood.w_dir_wall = true;
							}
							placeLibPart (insCell);
							placeLibPart (insCellB);
						}

						// ���� �� ������ �� ħ���� ���
						if ( (beamLeftX <= cellLeftX) && (cellRightX <= beamRightX) ) {
							// �������� ����
						}
					}
				}
			}
		}

		return NoError;
	});

	return err;
}

// �ش� �� ������ ������� ���̺귯�� ��ġ
API_Guid	placeLibPart (Cell objInfo)
{
	GSErrCode	err = NoError;

	API_Element			element;
	API_ElementMemo		memo;
	API_LibPart			libPart;

	const	GS::uchar_t* gsmName = NULL;
	double	aParam;
	double	bParam;
	Int32	addParNum;

	std::string		tempString;

	// GUID ���� �ʱ�ȭ
	element.header.guid.clock_seq_hi_and_reserved = 0;
	element.header.guid.clock_seq_low = 0;
	element.header.guid.node[0] = 0;
	element.header.guid.node[1] = 0;
	element.header.guid.node[2] = 0;
	element.header.guid.node[3] = 0;
	element.header.guid.node[4] = 0;
	element.header.guid.node[5] = 0;
	element.header.guid.time_hi_and_version = 0;
	element.header.guid.time_low = 0;
	element.header.guid.time_mid = 0;

	// ���̺귯�� �̸� ����
	if (objInfo.objType == NONE)			return element.header.guid;
	if (objInfo.objType == INCORNER)		gsmName = L("���ڳ��ǳ�v1.0.gsm");
	if (objInfo.objType == EUROFORM)		gsmName = L("������v2.0.gsm");
	if (objInfo.objType == FILLERSPACER)	gsmName = L("�ٷ������̼�v1.0.gsm");
	if (objInfo.objType == PLYWOOD)			gsmName = L("����v1.0.gsm");
	if (objInfo.objType == WOOD)			gsmName = L("����v1.0.gsm");

	// ��ü �ε�
	BNZeroMemory (&libPart, sizeof (libPart));
	GS::ucscpy (libPart.file_UName, gsmName);
	err = ACAPI_LibPart_Search (&libPart, false);
	if (err != NoError)
		return element.header.guid;
	if (libPart.location != NULL)
		delete libPart.location;

	ACAPI_LibPart_Get (&libPart);

	BNZeroMemory (&element, sizeof (API_Element));
	BNZeroMemory (&memo, sizeof (API_ElementMemo));

	element.header.typeID = API_ObjectID;
	element.header.guid = GSGuid2APIGuid (GS::Guid (libPart.ownUnID));

	ACAPI_Element_GetDefaults (&element, &memo);
	ACAPI_LibPart_GetParams (libPart.index, &aParam, &bParam, &addParNum, &memo.params);

	// ���̺귯���� �Ķ���� �� �Է�
	element.object.libInd = libPart.index;
	element.object.pos.x = objInfo.leftBottomX;
	element.object.pos.y = objInfo.leftBottomY;
	element.object.level = objInfo.leftBottomZ;
	element.object.xRatio = aParam;
	element.object.yRatio = bParam;
	element.object.angle = objInfo.ang;
	element.header.layer = layerInd;
	element.header.floorInd = infoWall.floorInd;

	if (objInfo.objType == INCORNER) {
		memo.params [0][27].value.real = objInfo.libPart.incorner.wid_s;	// ����(����)
		memo.params [0][28].value.real = objInfo.libPart.incorner.leng_s;	// ����(�Ķ�)
		memo.params [0][29].value.real = objInfo.libPart.incorner.hei_s;	// ����
		GS::ucscpy (memo.params [0][30].value.uStr, L("�����"));			// ��ġ����

	} else if (objInfo.objType == EUROFORM) {
		// �԰�ǰ�� ���,
		if (objInfo.libPart.form.eu_stan_onoff == true) {
			// �԰��� On/Off
			memo.params [0][27].value.real = TRUE;

			// �ʺ�
			tempString = format_string ("%.0f", objInfo.libPart.form.eu_wid * 1000);
			GS::ucscpy (memo.params [0][28].value.uStr, GS::UniString (tempString.c_str ()).ToUStr ().Get ());

			// ����
			tempString = format_string ("%.0f", objInfo.libPart.form.eu_hei * 1000);
			GS::ucscpy (memo.params [0][29].value.uStr, GS::UniString (tempString.c_str ()).ToUStr ().Get ());

		// ��԰�ǰ�� ���,
		} else {
			// �԰��� On/Off
			memo.params [0][27].value.real = FALSE;

			// �ʺ�
			memo.params [0][30].value.real = objInfo.libPart.form.eu_wid2;

			// ����
			memo.params [0][31].value.real = objInfo.libPart.form.eu_hei2;
		}

		// ��ġ����
		if (objInfo.libPart.form.u_ins_wall == true) {
			tempString = "�������";
		} else {
			tempString = "��������";
			element.object.pos.x += ( objInfo.horLen * cos(objInfo.ang) );
			element.object.pos.y += ( objInfo.horLen * sin(objInfo.ang) );
		}
		GS::ucscpy (memo.params [0][32].value.uStr, GS::UniString (tempString.c_str ()).ToUStr ().Get ());

	} else if (objInfo.objType == FILLERSPACER) {
		memo.params [0][27].value.real = objInfo.libPart.fillersp.f_thk;	// �β�
		memo.params [0][28].value.real = objInfo.libPart.fillersp.f_leng;	// ����
		element.object.pos.x += ( objInfo.libPart.fillersp.f_thk * cos(objInfo.ang) );
		element.object.pos.y += ( objInfo.libPart.fillersp.f_thk * sin(objInfo.ang) );

	} else if (objInfo.objType == PLYWOOD) {
		GS::ucscpy (memo.params [0][32].value.uStr, L("��԰�"));
		memo.params [0][35].value.real = objInfo.libPart.fillersp.f_thk;	// ����
		memo.params [0][36].value.real = objInfo.libPart.fillersp.f_leng;	// ����
		
		// ��ġ����
		if (objInfo.libPart.plywood.w_dir_wall == true)
			tempString = "�������";
		else
			tempString = "��������";
		GS::ucscpy (memo.params [0][33].value.uStr, GS::UniString (tempString.c_str ()).ToUStr ().Get ());
	} else if (objInfo.objType == WOOD) {
		GS::ucscpy (memo.params [0][27].value.uStr, L("�������"));		// ��ġ����
		memo.params [0][28].value.real = objInfo.libPart.wood.w_w;		// �β�
		memo.params [0][29].value.real = objInfo.libPart.wood.w_h;		// �ʺ�
		memo.params [0][30].value.real = objInfo.libPart.wood.w_leng;	// ����
		memo.params [0][31].value.real = objInfo.libPart.wood.w_ang;	// ����

		// ���簡 ���η� ��� ��ġ�� ���
		if ( abs (RadToDegree (objInfo.libPart.wood.w_ang) - 90.0) < EPS ) {
			element.object.pos.x += ( objInfo.libPart.wood.w_h * cos(objInfo.ang) );
			element.object.pos.y += ( objInfo.libPart.wood.w_h * sin(objInfo.ang) );
		}
	}

	// ��ü ��ġ
	ACAPI_Element_Create (&element, &memo);
	ACAPI_DisposeElemMemoHdls (&memo);

	return element.header.guid;
}

// [arr1]�� - �ش� ���� ���ϴ� ��ǥX ��ġ�� ����
double	getCellPositionLeftBottomX (PlacingZone *src_zone, short arr1, short idx)
{
	double		distance = 0.0;
	short		xx;

	for (xx = 0 ; xx < idx ; ++xx) {
		if (src_zone->cells [arr1][xx].objType != NONE)
			distance += src_zone->cells [arr1][xx].horLen;
	}

	return distance;
}

// [arr1]�� - ��ü ���� ���ϴ� ��ǥZ ��ġ�� ����
void	setCellPositionLeftBottomZ (PlacingZone *src_zone, short arr1, double new_hei)
{
	short		xx;

	for (xx = 0 ; xx < src_zone->nCells ; ++xx)
		src_zone->cells [arr1][xx].leftBottomZ = new_hei;
}

// Cell �迭�� �ʱ�ȭ��
void	initCellsForWall (PlacingZone* placingZone)
{
	short xx, yy;

	for (xx = 0 ; xx < 50 ; ++xx)
		for (yy = 0 ; yy < 100 ; ++yy) {
			placingZone->cells [xx][yy].objType = NONE;
			placingZone->cells [xx][yy].ang = 0.0;
			placingZone->cells [xx][yy].horLen = 0.0;
			placingZone->cells [xx][yy].verLen = 0.0;
			placingZone->cells [xx][yy].leftBottomX = 0.0;
			placingZone->cells [xx][yy].leftBottomY = 0.0;
			placingZone->cells [xx][yy].leftBottomZ = 0.0;
		}
}

// 1�� ��ġ: ���ڳ�, ������
void	firstPlacingSettingsForWall (PlacingZone* placingZone)
{
	short			xx, yy, zz;
	std::string		tempString;

	// ���� ���ڳ� ����
	if (placingZone->bLIncorner) {
		for (xx = 0 ; xx <= placingZone->eu_count_ver ; ++xx) {
			placingZone->cells [xx][0].objType = INCORNER;
			placingZone->cells [xx][0].horLen = placingZone->lenLIncorner;
			if (placingZone->eu_ori.compare (std::string ("�������")) == 0)
				placingZone->cells [xx][0].verLen = placingZone->eu_hei_numeric;
			else
				placingZone->cells [xx][0].verLen = placingZone->eu_wid_numeric;
			placingZone->cells [xx][0].ang = placingZone->ang + DegreeToRad (-90);
			placingZone->cells [xx][0].leftBottomX = placingZone->leftBottomX;
			placingZone->cells [xx][0].leftBottomY = placingZone->leftBottomY;
			placingZone->cells [xx][0].leftBottomZ = placingZone->leftBottomZ + (xx * placingZone->cells [0][0].verLen);
			placingZone->cells [xx][0].libPart.incorner.wid_s = 0.100;									// ���ڳ��г� - ����(����)
			placingZone->cells [xx][0].libPart.incorner.leng_s = placingZone->lenLIncorner;				// ���ڳ��г� - ����(�Ķ�)
			if (placingZone->eu_ori.compare (std::string ("�������")) == 0)
				placingZone->cells [xx][0].libPart.incorner.hei_s = placingZone->eu_hei_numeric;		// ���ڳ��г� - ����
			else
				placingZone->cells [xx][0].libPart.incorner.hei_s = placingZone->eu_wid_numeric;		// ���ڳ��г� - ����
		}
	}

	// ������ ����
	for (xx = 0 ; xx <= placingZone->eu_count_ver ; ++xx) {
		for (yy = 1 ; yy <= placingZone->eu_count_hor ; ++yy) {
			zz = yy * 2;	// Cell�� ��ü ������ ä�� (�ε���: 2, 4, 6 ������)

			placingZone->cells [xx][zz].objType = EUROFORM;
			placingZone->cells [xx][zz].ang = placingZone->ang;

			if (placingZone->eu_ori.compare (std::string ("�������")) == 0) {
				placingZone->cells [xx][zz].libPart.form.u_ins_wall = true;
				placingZone->cells [xx][zz].horLen = placingZone->eu_wid_numeric;
				placingZone->cells [xx][zz].verLen = placingZone->eu_hei_numeric;
			} else {
				placingZone->cells [xx][zz].libPart.form.u_ins_wall = false;
				placingZone->cells [xx][zz].horLen = placingZone->eu_hei_numeric;
				placingZone->cells [xx][zz].verLen = placingZone->eu_wid_numeric;
			}

			placingZone->cells [xx][zz].leftBottomX = placingZone->leftBottomX + (getCellPositionLeftBottomX (placingZone, xx, zz) * cos(placingZone->ang));
			placingZone->cells [xx][zz].leftBottomY = placingZone->leftBottomY + (getCellPositionLeftBottomX (placingZone, xx, zz) * sin(placingZone->ang));
			placingZone->cells [xx][zz].leftBottomZ = placingZone->leftBottomZ + (xx * placingZone->cells [0][zz].verLen);
			placingZone->cells [xx][zz].libPart.form.eu_stan_onoff = true;
			placingZone->cells [xx][zz].libPart.form.eu_wid = placingZone->eu_wid_numeric;
			placingZone->cells [xx][zz].libPart.form.eu_hei = placingZone->eu_hei_numeric;
		}
	}

	// ������ ���ڳ� ����
	if (placingZone->bRIncorner) {
		for (xx = 0 ; xx <= placingZone->eu_count_ver ; ++xx) {
			placingZone->cells [xx][placingZone->nCells - 1].objType = INCORNER;
			placingZone->cells [xx][placingZone->nCells - 1].horLen = placingZone->lenRIncorner;
			if (placingZone->eu_ori.compare (std::string ("�������")) == 0)
				placingZone->cells [xx][placingZone->nCells - 1].verLen = placingZone->eu_hei_numeric;
			else
				placingZone->cells [xx][placingZone->nCells - 1].verLen = placingZone->eu_wid_numeric;
			placingZone->cells [xx][placingZone->nCells - 1].ang = placingZone->ang + DegreeToRad (-180);
			placingZone->cells [xx][placingZone->nCells - 1].leftBottomX = placingZone->leftBottomX + ((getCellPositionLeftBottomX (placingZone, xx, placingZone->nCells - 1) + placingZone->lenRIncorner) * cos(placingZone->ang));
			placingZone->cells [xx][placingZone->nCells - 1].leftBottomY = placingZone->leftBottomY + ((getCellPositionLeftBottomX (placingZone, xx, placingZone->nCells - 1) + placingZone->lenRIncorner) * sin(placingZone->ang));
			placingZone->cells [xx][placingZone->nCells - 1].leftBottomZ = placingZone->leftBottomZ + (xx * placingZone->cells [0][placingZone->nCells - 1].verLen);
			placingZone->cells [xx][placingZone->nCells - 1].libPart.incorner.wid_s = placingZone->lenRIncorner;			// ���ڳ��г� - ����(����)
			placingZone->cells [xx][placingZone->nCells - 1].libPart.incorner.leng_s = 0.100;								// ���ڳ��г� - ����(�Ķ�)
			if (placingZone->eu_ori.compare (std::string ("�������")) == 0)
				placingZone->cells [xx][placingZone->nCells - 1].libPart.incorner.hei_s = placingZone->eu_hei_numeric;		// ���ڳ��г� - ����
			else
				placingZone->cells [xx][placingZone->nCells - 1].libPart.incorner.hei_s = placingZone->eu_wid_numeric;		// ���ڳ��г� - ����
		}
	}
}

// ���� ���� ���� ������ ��Ī�ϴ� �ݴ��ʿ��� ������
void	copyPlacingZoneSymmetricForWall (PlacingZone* src_zone, PlacingZone* dst_zone, InfoWall* infoWall)
{
	short	xx, yy;

	// ���� ���� ���� �ʱ�ȭ
	dst_zone->leftBottomX			= src_zone->leftBottomX - (infoWall->wallThk * sin(src_zone->ang));
	dst_zone->leftBottomY			= src_zone->leftBottomY + (infoWall->wallThk * cos(src_zone->ang));
	dst_zone->leftBottomZ			= src_zone->leftBottomZ;
	dst_zone->horLen				= src_zone->horLen;
	dst_zone->verLen				= src_zone->verLen;
	dst_zone->ang					= src_zone->ang;

	dst_zone->nInterfereBeams		= src_zone->nInterfereBeams;
	for (xx = 0 ; xx < 30 ; ++xx)
		dst_zone->beams [xx]			= src_zone->beams [xx];

	dst_zone->remain_hor			= src_zone->remain_hor;
	dst_zone->remain_hor_updated	= src_zone->remain_hor_updated;
	dst_zone->remain_ver			= src_zone->remain_ver;
	dst_zone->remain_ver_wo_beams	= src_zone->remain_ver_wo_beams;

	dst_zone->bLIncorner			= src_zone->bLIncorner;
	dst_zone->bRIncorner			= src_zone->bRIncorner;
	dst_zone->lenLIncorner			= src_zone->lenLIncorner;
	dst_zone->lenRIncorner			= src_zone->lenRIncorner;

	dst_zone->eu_wid				= src_zone->eu_wid;
	dst_zone->eu_wid_numeric		= src_zone->eu_wid_numeric;
	dst_zone->eu_hei				= src_zone->eu_hei;
	dst_zone->eu_hei_numeric		= src_zone->eu_hei_numeric;
	dst_zone->eu_ori				= src_zone->eu_ori;
	dst_zone->eu_count_hor			= src_zone->eu_count_hor;
	dst_zone->eu_count_ver			= src_zone->eu_count_ver;

	dst_zone->nCells				= src_zone->nCells;


	// Cell ������ ��Ī������ ����
	for (xx = 0 ; xx < dst_zone->eu_count_ver ; ++xx) {
		for (yy = 0 ; yy < dst_zone->nCells ; ++yy) {

			// �ƹ��͵� ������,
			if (src_zone->cells [xx][yy].objType == NONE) {

				dst_zone->cells [xx][yy].objType			= NONE;
				dst_zone->cells [xx][yy].horLen				= 0;
				dst_zone->cells [xx][yy].verLen				= 0;
				dst_zone->cells [xx][yy].ang				= dst_zone->ang;
				dst_zone->cells [xx][yy].leftBottomX		= dst_zone->leftBottomX + (getCellPositionLeftBottomX (src_zone, xx, yy) * cos(src_zone->ang));
				dst_zone->cells [xx][yy].leftBottomY		= dst_zone->leftBottomY + (getCellPositionLeftBottomX (src_zone, xx, yy) * sin(src_zone->ang));
				dst_zone->cells [xx][yy].leftBottomZ		= src_zone->leftBottomZ;

			// ���ڳ��� ���,
			} else if (src_zone->cells [xx][yy].objType == INCORNER) {
				// ���� ���ڳ�
				if ((placingZoneBackside.bLIncorner) && (yy == 0)) {
					dst_zone->cells [xx][yy].objType						= INCORNER;
					dst_zone->cells [xx][yy].horLen							= src_zone->cells [xx][yy].horLen;
					dst_zone->cells [xx][yy].verLen							= src_zone->cells [xx][yy].verLen;
					dst_zone->cells [xx][yy].ang							= dst_zone->ang;
					dst_zone->cells [xx][yy].leftBottomX					= dst_zone->leftBottomX;
					dst_zone->cells [xx][yy].leftBottomY					= dst_zone->leftBottomY;
					dst_zone->cells [xx][yy].leftBottomZ					= src_zone->cells [xx][yy].leftBottomZ;
					dst_zone->cells [xx][yy].libPart.incorner.wid_s			= src_zone->cells [xx][yy].libPart.incorner.leng_s;		// ���ڳ��г� - ����(����)
					dst_zone->cells [xx][yy].libPart.incorner.leng_s		= src_zone->cells [xx][yy].libPart.incorner.wid_s;		// ���ڳ��г� - ����(�Ķ�)
					dst_zone->cells [xx][yy].libPart.incorner.hei_s			= src_zone->cells [xx][yy].libPart.incorner.hei_s;		// ���ڳ��г� - ����

				// ������ ���ڳ�
				} else if ((placingZoneBackside.bRIncorner) && (yy == src_zone->nCells - 1)) {
					dst_zone->cells [xx][yy].objType						= INCORNER;
					dst_zone->cells [xx][yy].horLen							= src_zone->cells [xx][yy].horLen;
					dst_zone->cells [xx][yy].verLen							= src_zone->cells [xx][yy].verLen;
					dst_zone->cells [xx][yy].ang							= dst_zone->ang + DegreeToRad (90);
					dst_zone->cells [xx][yy].leftBottomX					= dst_zone->leftBottomX + ((getCellPositionLeftBottomX (src_zone, xx, yy) + src_zone->lenRIncorner) * cos(src_zone->ang));
					dst_zone->cells [xx][yy].leftBottomY					= dst_zone->leftBottomY + ((getCellPositionLeftBottomX (src_zone, xx, yy) + src_zone->lenRIncorner) * sin(src_zone->ang));
					dst_zone->cells [xx][yy].leftBottomZ					= src_zone->cells [xx][yy].leftBottomZ;
					dst_zone->cells [xx][yy].libPart.incorner.wid_s			= src_zone->cells [xx][yy].libPart.incorner.leng_s;		// ���ڳ��г� - ����(����)
					dst_zone->cells [xx][yy].libPart.incorner.leng_s		= src_zone->cells [xx][yy].libPart.incorner.wid_s;		// ���ڳ��г� - ����(�Ķ�)
					dst_zone->cells [xx][yy].libPart.incorner.hei_s			= src_zone->cells [xx][yy].libPart.incorner.hei_s;		// ���ڳ��г� - ����

				// ��Ÿ ��ġ
				} else {
					dst_zone->cells [xx][yy].objType						= INCORNER;
					dst_zone->cells [xx][yy].horLen							= src_zone->cells [xx][yy].horLen;
					dst_zone->cells [xx][yy].verLen							= src_zone->cells [xx][yy].verLen;
					dst_zone->cells [xx][yy].ang							= dst_zone->ang + DegreeToRad (90);
					dst_zone->cells [xx][yy].leftBottomX					= dst_zone->leftBottomX + ((getCellPositionLeftBottomX (src_zone, xx, yy) + src_zone->lenRIncorner) * cos(src_zone->ang));
					dst_zone->cells [xx][yy].leftBottomY					= dst_zone->leftBottomY + ((getCellPositionLeftBottomX (src_zone, xx, yy) + src_zone->lenRIncorner) * sin(src_zone->ang));
					dst_zone->cells [xx][yy].leftBottomZ					= src_zone->cells [xx][yy].leftBottomZ;
					dst_zone->cells [xx][yy].libPart.incorner.wid_s			= src_zone->cells [xx][yy].libPart.incorner.leng_s;		// ���ڳ��г� - ����(����)
					dst_zone->cells [xx][yy].libPart.incorner.leng_s		= src_zone->cells [xx][yy].libPart.incorner.wid_s;		// ���ڳ��г� - ����(�Ķ�)
					dst_zone->cells [xx][yy].libPart.incorner.hei_s			= src_zone->cells [xx][yy].libPart.incorner.hei_s;		// ���ڳ��г� - ����
				}
		
			// �������� ���,
			} else if (src_zone->cells [xx][yy].objType == EUROFORM) {
		
				dst_zone->cells [xx][yy].objType					= EUROFORM;
				dst_zone->cells [xx][yy].horLen						= src_zone->cells [xx][yy].horLen;
				dst_zone->cells [xx][yy].verLen						= src_zone->cells [xx][yy].verLen;
				dst_zone->cells [xx][yy].ang						= dst_zone->ang + DegreeToRad (180);

				dst_zone->cells [xx][yy].leftBottomX = dst_zone->leftBottomX + ((getCellPositionLeftBottomX (src_zone, xx, yy) + src_zone->cells [xx][yy].horLen) * cos(src_zone->ang));
				dst_zone->cells [xx][yy].leftBottomY = dst_zone->leftBottomY + ((getCellPositionLeftBottomX (src_zone, xx, yy) + src_zone->cells [xx][yy].horLen) * sin(src_zone->ang));
				dst_zone->cells [xx][yy].leftBottomZ					= src_zone->cells [xx][yy].leftBottomZ;
				dst_zone->cells [xx][yy].libPart.form.eu_stan_onoff		= src_zone->cells [xx][yy].libPart.form.eu_stan_onoff;
				dst_zone->cells [xx][yy].libPart.form.eu_wid			= src_zone->cells [xx][yy].libPart.form.eu_wid;
				dst_zone->cells [xx][yy].libPart.form.eu_hei			= src_zone->cells [xx][yy].libPart.form.eu_hei;
				dst_zone->cells [xx][yy].libPart.form.eu_wid2			= src_zone->cells [xx][yy].libPart.form.eu_wid2;
				dst_zone->cells [xx][yy].libPart.form.eu_hei2			= src_zone->cells [xx][yy].libPart.form.eu_hei2;
				dst_zone->cells [xx][yy].libPart.form.u_ins_wall		= src_zone->cells [xx][yy].libPart.form.u_ins_wall;

			// �ٷ������̼��� ���,
			} else if (src_zone->cells [xx][yy].objType == FILLERSPACER) {
		
				dst_zone->cells [xx][yy].objType			= FILLERSPACER;
				dst_zone->cells [xx][yy].horLen				= src_zone->cells [xx][yy].horLen;
				dst_zone->cells [xx][yy].verLen				= src_zone->cells [xx][yy].verLen;
				dst_zone->cells [xx][yy].ang				= dst_zone->ang + DegreeToRad (180);
				dst_zone->cells [xx][yy].leftBottomX		= dst_zone->leftBottomX + ((getCellPositionLeftBottomX (src_zone, xx, yy) + src_zone->cells [xx][yy].horLen) * cos(src_zone->ang));
				dst_zone->cells [xx][yy].leftBottomY		= dst_zone->leftBottomY + ((getCellPositionLeftBottomX (src_zone, xx, yy) + src_zone->cells [xx][yy].horLen) * sin(src_zone->ang));
				dst_zone->cells [xx][yy].leftBottomZ		= src_zone->cells [xx][yy].leftBottomZ;
				dst_zone->cells [xx][yy].libPart.fillersp.f_leng	= src_zone->cells [xx][yy].libPart.fillersp.f_leng;
				dst_zone->cells [xx][yy].libPart.fillersp.f_thk		= src_zone->cells [xx][yy].libPart.fillersp.f_thk;

			// ������ ���,
			} else if (src_zone->cells [xx][yy].objType == PLYWOOD) {

				dst_zone->cells [xx][yy].objType			= PLYWOOD;
				dst_zone->cells [xx][yy].horLen				= src_zone->cells [xx][yy].horLen;
				dst_zone->cells [xx][yy].verLen				= src_zone->cells [xx][yy].verLen;
				dst_zone->cells [xx][yy].ang				= dst_zone->ang + DegreeToRad (180);
				dst_zone->cells [xx][yy].leftBottomX		= dst_zone->leftBottomX + ((getCellPositionLeftBottomX (src_zone, xx, yy) + src_zone->cells [xx][yy].horLen) * cos(src_zone->ang));
				dst_zone->cells [xx][yy].leftBottomY		= dst_zone->leftBottomY + ((getCellPositionLeftBottomX (src_zone, xx, yy) + src_zone->cells [xx][yy].horLen) * sin(src_zone->ang));
				dst_zone->cells [xx][yy].leftBottomZ		= src_zone->cells [xx][yy].leftBottomZ;
				dst_zone->cells [xx][yy].libPart.plywood.p_leng		= src_zone->cells [xx][yy].libPart.plywood.p_leng;
				dst_zone->cells [xx][yy].libPart.plywood.p_wid		= src_zone->cells [xx][yy].libPart.plywood.p_wid;
				dst_zone->cells [xx][yy].libPart.plywood.w_dir_wall	= src_zone->cells [xx][yy].libPart.plywood.w_dir_wall;
		
			// ������ ���,
			} else if (src_zone->cells [xx][yy].objType == WOOD) {

				dst_zone->cells [xx][yy].objType			= WOOD;
				dst_zone->cells [xx][yy].horLen				= src_zone->cells [xx][yy].horLen;
				dst_zone->cells [xx][yy].verLen				= src_zone->cells [xx][yy].verLen;
				dst_zone->cells [xx][yy].ang				= dst_zone->ang + DegreeToRad (180);
				dst_zone->cells [xx][yy].leftBottomX		= dst_zone->leftBottomX + ((getCellPositionLeftBottomX (src_zone, xx, yy) + src_zone->cells [xx][yy].horLen) * cos(src_zone->ang));
				dst_zone->cells [xx][yy].leftBottomY		= dst_zone->leftBottomY + ((getCellPositionLeftBottomX (src_zone, xx, yy) + src_zone->cells [xx][yy].horLen) * sin(src_zone->ang));
				dst_zone->cells [xx][yy].leftBottomZ		= src_zone->cells [xx][yy].leftBottomZ;
				dst_zone->cells [xx][yy].libPart.wood.w_h		= src_zone->cells [xx][yy].libPart.wood.w_h;
				dst_zone->cells [xx][yy].libPart.wood.w_leng	= src_zone->cells [xx][yy].libPart.wood.w_leng;
				dst_zone->cells [xx][yy].libPart.wood.w_w		= src_zone->cells [xx][yy].libPart.wood.w_w;
				dst_zone->cells [xx][yy].libPart.wood.w_ang		= src_zone->cells [xx][yy].libPart.wood.w_ang;
			}
		}
	}
}

// Cell ������ ����ʿ� ���� ����ȭ�� ��ġ�� ��������
void	alignPlacingZoneForWall (PlacingZone* target_zone)
{
	short			xx, yy;

	// �� Cell���� ��ġ �� ���� ������ ������Ʈ��
	for (xx = 0 ; xx < target_zone->eu_count_ver ; ++xx) {
		for (yy = 0 ; yy < target_zone->nCells ; ++yy) {
			// ���ڳ��� ���,
			if (target_zone->cells [xx][yy].objType == INCORNER) {

				if (yy == 0) {
					target_zone->cells [xx][yy].ang			= target_zone->ang + DegreeToRad (-90);
					target_zone->cells [xx][yy].leftBottomX	= target_zone->leftBottomX;
					target_zone->cells [xx][yy].leftBottomY	= target_zone->leftBottomY;
				} else {
					target_zone->cells [xx][yy].ang			= target_zone->ang + DegreeToRad (-180);
					target_zone->cells [xx][yy].leftBottomX	= target_zone->leftBottomX + ((getCellPositionLeftBottomX (target_zone, xx, yy) + target_zone->lenRIncorner) * cos(target_zone->ang));
					target_zone->cells [xx][yy].leftBottomY	= target_zone->leftBottomY + ((getCellPositionLeftBottomX (target_zone, xx, yy) + target_zone->lenRIncorner) * sin(target_zone->ang));
				}
				target_zone->cells [xx][yy].leftBottomZ = target_zone->leftBottomZ + (target_zone->cells [0][yy].verLen * xx);

			// ������: NONE, ������, �ٷ������̼�, ����, ������ ���
			} else {

				target_zone->cells [xx][yy].ang			= target_zone->ang;
				target_zone->cells [xx][yy].leftBottomX	= target_zone->leftBottomX + (getCellPositionLeftBottomX (target_zone, xx, yy) * cos(target_zone->ang));;
				target_zone->cells [xx][yy].leftBottomY	= target_zone->leftBottomY + (getCellPositionLeftBottomX (target_zone, xx, yy) * sin(target_zone->ang));;
				target_zone->cells [xx][yy].leftBottomZ = target_zone->leftBottomZ + (target_zone->cells [0][yy].verLen * xx);
			}
		}
	}

	// ���� �������� ���� �Ÿ� ���� �׸���� ������Ʈ��
	target_zone->remain_hor_updated = target_zone->remain_hor = target_zone->horLen;

	// ���� ���� ���� ����: �� ���� �ʺ�ŭ ����
	for (yy = 0 ; yy < target_zone->nCells ; ++yy) {
		if (target_zone->cells [0][yy].objType != NONE) {
			target_zone->remain_hor_updated -= target_zone->cells [0][yy].horLen;
		}
	}
}

// src���� Cell ��ü ������ dst������ ����
void	copyCellsToAnotherLine (PlacingZone* target_zone, short src_row, short dst_row)
{
	short xx;

	for (xx = 0 ; xx <= target_zone->nCells ; ++xx)
		target_zone->cells [dst_row][xx] = target_zone->cells [src_row][xx];
}

// 1�� ��ġ�� ���� ���Ǹ� ��û�ϴ� 1�� ���̾�α�
short DGCALLBACK wallPlacerHandlerPrimary (short message, short dialogID, short item, DGUserData /* userData */, DGMessageData /* msgData */)
{
	short		result;
	API_UCCallbackType	ucb;

	switch (message) {
		case DG_MSG_INIT:
			// ���̾�α� Ÿ��Ʋ
			DGSetDialogTitle (dialogID, "������/���ڳ�/��Ÿ ��ġ - �⺻ ��ġ");

			//////////////////////////////////////////////////////////// ������ ��ġ (�⺻ ��ư)
			// ���� ��ư
			DGSetItemText (dialogID, DG_OK, "Ȯ  ��");

			// ���� ��ư
			DGSetItemText (dialogID, DG_CANCEL, "��  ��");

			//////////////////////////////////////////////////////////// ������ ��ġ (���ڳ� ����)
			// ��: ���ڳ� ��ġ ����
			DGSetItemText (dialogID, LABEL_INCORNER, "���ڳ� ��ġ ����");

			// üũ�ڽ�: ���� ���ڳ�
			DGSetItemText (dialogID, CHECKBOX_SET_LEFT_INCORNER, "����");
			DGSetItemValLong (dialogID, CHECKBOX_SET_LEFT_INCORNER, true);

			// Edit ��Ʈ��: ���� ���ڳ�
			DGSetItemValDouble (dialogID, EDITCONTROL_LEFT_INCORNER, 0.100);

			// üũ�ڽ�: ������ ���ڳ�
			DGSetItemText (dialogID, CHECKBOX_SET_RIGHT_INCORNER, "������");
			DGSetItemValLong (dialogID, CHECKBOX_SET_RIGHT_INCORNER, true);

			// Edit ��Ʈ��: ������ ���ڳ�
			DGSetItemValDouble (dialogID, EDITCONTROL_RIGHT_INCORNER, 0.100);

			//////////////////////////////////////////////////////////// ������ ��ġ (������)
			// ��: ������ ��ġ ����
			DGSetItemText (dialogID, LABEL_PLACING_EUROFORM, "������ ��ġ ����");

			// ��: �ʺ�
			DGSetItemText (dialogID, LABEL_EUROFORM_WIDTH, "�ʺ�");

			// ��: ����
			DGSetItemText (dialogID, LABEL_EUROFORM_HEIGHT, "����");

			// ��: ��ġ����
			DGSetItemText (dialogID, LABEL_EUROFORM_ORIENTATION, "��ġ����");

			// ���� ��Ʈ�� �ʱ�ȭ
			BNZeroMemory (&ucb, sizeof (ucb));
			ucb.dialogID = dialogID;
			ucb.type	 = APIUserControlType_Layer;
			ucb.itemID	 = USERCONTROL_LAYER;
			ACAPI_Interface (APIIo_SetUserControlCallbackID, &ucb, NULL);
			DGSetItemValLong (dialogID, USERCONTROL_LAYER, 1);

			break;

		case DG_MSG_CHANGE:
			// üũ�ڽ� ���� (���ڳ� ����)
			if (DGGetItemValLong (dialogID, CHECKBOX_SET_LEFT_INCORNER) == 1)
				DGEnableItem (dialogID, EDITCONTROL_LEFT_INCORNER);
			else
				DGDisableItem (dialogID, EDITCONTROL_LEFT_INCORNER);

			if (DGGetItemValLong (dialogID, CHECKBOX_SET_RIGHT_INCORNER) == 1)
				DGEnableItem (dialogID, EDITCONTROL_RIGHT_INCORNER);
			else
				DGDisableItem (dialogID, EDITCONTROL_RIGHT_INCORNER);

			break;

		case DG_MSG_CLICK:
			switch (item) {
				case DG_OK:
					//////////////////////////////////////// ���̾�α� â ������ �Է� ����
					// ������ �ʺ�, ����, ����
					placingZone.eu_wid = DGPopUpGetItemText (dialogID, POPUP_EUROFORM_WIDTH, static_cast<short>(DGGetItemValLong (dialogID, POPUP_EUROFORM_WIDTH))).ToCStr ().Get ();
					placingZone.eu_hei = DGPopUpGetItemText (dialogID, POPUP_EUROFORM_HEIGHT, static_cast<short>(DGGetItemValLong (dialogID, POPUP_EUROFORM_HEIGHT))).ToCStr ().Get ();
					placingZone.eu_ori = DGPopUpGetItemText (dialogID, POPUP_EUROFORM_ORIENTATION, static_cast<short>(DGGetItemValLong (dialogID, POPUP_EUROFORM_ORIENTATION))).ToCStr ().Get ();

					// �¿� ���ڳ� ����
					if (DGGetItemValLong (dialogID, CHECKBOX_SET_LEFT_INCORNER) == TRUE)
						placingZone.bLIncorner = true;
					else
						placingZone.bLIncorner = false;
					if (DGGetItemValLong (dialogID, CHECKBOX_SET_RIGHT_INCORNER) == TRUE)
						placingZone.bRIncorner = true;
					else
						placingZone.bRIncorner = false;
					placingZone.lenLIncorner = DGGetItemValDouble (dialogID, EDITCONTROL_LEFT_INCORNER);
					placingZone.lenRIncorner = DGGetItemValDouble (dialogID, EDITCONTROL_RIGHT_INCORNER);

					// ���̾� ��ȣ ����
					layerInd = (short)DGGetItemValLong (dialogID, USERCONTROL_LAYER);

					break;
				case DG_CANCEL:
					break;
			}
		case DG_MSG_CLOSE:
			switch (item) {
				case DG_CLOSEBOX:
					break;
			}
	}

	result = item;

	return	result;
}

// 1�� ��ġ �� ������ ��û�ϴ� 2�� ���̾�α�
short DGCALLBACK wallPlacerHandlerSecondary (short message, short dialogID, short item, DGUserData /* userData */, DGMessageData /* msgData */)
{
	short	result;
	short	btnSizeX = 50, btnSizeY = 50;
	short	dialogSizeX, dialogSizeY;
	short	groupboxSizeX, groupboxSizeY;
	short	btnInitPosX = 220;
	short	btnPosX = 220, btnPosY = (btnSizeY * placingZone.eu_count_ver);
	short	xx, yy;
	short	idxBtn;
	short	lastIdxBtn;
	short	idxCell;
	short	idxCell_prev = -1, idxCell_next = -1;
	std::string		txtButton = "";
	API_Element		elem;
	GSErrCode		err;

	switch (message) {
		case DG_MSG_INIT:
			// ���̾�α� Ÿ��Ʋ
			DGSetDialogTitle (dialogID, "������/���ڳ�/��Ÿ ��ġ - ���� ä���");

			//////////////////////////////////////////////////////////// ������ ��ġ (�⺻ ��ư)
			// ������Ʈ ��ư
			DGAppendDialogItem (dialogID, DG_ITM_BUTTON, DG_BT_ICONTEXT, 0, 40, 100, 130, 25);
			DGSetItemFont (dialogID, DG_OK, DG_IS_LARGE | DG_IS_PLAIN);
			DGSetItemText (dialogID, DG_OK, "2. ��  ġ");
			DGShowItem (dialogID, DG_OK);

			// ���� ��ư
			DGAppendDialogItem (dialogID, DG_ITM_BUTTON, DG_BT_ICONTEXT, 0, 40, 140, 130, 25);
			DGSetItemFont (dialogID, DG_CANCEL, DG_IS_LARGE | DG_IS_PLAIN);
			DGSetItemText (dialogID, DG_CANCEL, "3. ������ ä���");
			DGShowItem (dialogID, DG_CANCEL);

			//////////////////////////////////////////////////////////// ������ ��ġ (���ڳ� ����)
			// ��: ���� ���� ����
			DGAppendDialogItem (dialogID, DG_ITM_STATICTEXT, DG_IS_LEFT, DG_FT_NONE, 10, 15, 90, 23);
			DGSetItemFont (dialogID, LABEL_REMAIN_HORIZONTAL_LENGTH, DG_IS_LARGE | DG_IS_BOLD);
			DGSetItemText (dialogID, LABEL_REMAIN_HORIZONTAL_LENGTH, "���� ���� ����");
			DGShowItem (dialogID, LABEL_REMAIN_HORIZONTAL_LENGTH);

			// Edit ��Ʈ��: ���� ���� ����
			DGAppendDialogItem (dialogID, DG_ITM_EDITTEXT, DG_ET_LENGTH, 0, 110, 15-7, 50, 25);
			DGSetItemFont (dialogID, EDITCONTROL_REMAIN_HORIZONTAL_LENGTH, DG_IS_LARGE | DG_IS_BOLD);
			DGShowItem (dialogID, EDITCONTROL_REMAIN_HORIZONTAL_LENGTH);
			DGSetItemValDouble (dialogID, EDITCONTROL_REMAIN_HORIZONTAL_LENGTH, placingZone.remain_hor_updated);

			// �׷�ڽ�: ������/�ٷ������̼� ��ġ ����
			groupboxSizeX = 40 + (btnSizeX * (placingZone.eu_count_hor + 3));
			groupboxSizeY = 70 + (btnSizeY * placingZone.eu_count_ver);
			DGAppendDialogItem (dialogID, DG_ITM_GROUPBOX, DG_GT_PRIMARY, 0, 200, 10, groupboxSizeX, groupboxSizeY);
			DGSetItemFont (dialogID, GROUPBOX_GRID_EUROFORM_FILLERSPACER, DG_IS_LARGE | DG_IS_PLAIN);
			DGSetItemText (dialogID, GROUPBOX_GRID_EUROFORM_FILLERSPACER, "������/�ٷ������̼� ��ġ ����");
			DGShowItem (dialogID, GROUPBOX_GRID_EUROFORM_FILLERSPACER);

			// ���� �Ÿ� Ȯ�� ��ư
			DGAppendDialogItem (dialogID, DG_ITM_BUTTON, DG_BT_ICONTEXT, 0, 40, 60, 130, 25);
			DGSetItemFont (dialogID, PUSHBUTTON_CONFIRM_REMAIN_LENGTH, DG_IS_LARGE | DG_IS_BOLD);
			DGSetItemText (dialogID, PUSHBUTTON_CONFIRM_REMAIN_LENGTH, "1. ���� ���� Ȯ��");
			DGShowItem (dialogID, PUSHBUTTON_CONFIRM_REMAIN_LENGTH);

			// ���� â ũ�⸦ ����
			dialogSizeX = 270 + (btnSizeX * (placingZone.eu_count_hor + 3));
			dialogSizeY = max<short>(300, 150 + (btnSizeY * placingZone.eu_count_ver));
			DGSetDialogSize (dialogID, DG_FRAME, dialogSizeX, dialogSizeY, DG_CENTER, true);

			// �׸��� ����ü�� ���� ��ư�� �������� ��ġ��
			for (xx = 0 ; xx < placingZone.eu_count_ver ; ++xx) {
				for (yy = 0 ; yy < placingZone.nCells ; yy += 2) {
					idxBtn = DGAppendDialogItem (dialogID, DG_ITM_BUTTON, DG_BT_ICONTEXT, 0, btnPosX, btnPosY, btnSizeX, btnSizeY);
					lastIdxBtn = idxBtn;
					DGSetItemFont (dialogID, idxBtn, DG_IS_SMALL);

					idxCell = ((idxBtn - itemInitIdx) * 2) - (xx * (placingZone.eu_count_hor + 2)) * 2;		// ��ư �ε����� �� �ε����� ����

					txtButton = "";
					if (placingZone.cells [0][idxCell].objType == NONE) {
						txtButton = "NONE";
					} else if (placingZone.cells [0][idxCell].objType == INCORNER) {
						txtButton = format_string ("���ڳ�\n��%.0f\n��%.0f", placingZone.cells [0][idxCell].horLen * 1000, placingZone.cells [0][idxCell].verLen * 1000);
					} else if (placingZone.cells [0][idxCell].objType == EUROFORM) {
						if (placingZone.cells [0][idxCell].libPart.form.u_ins_wall)
							txtButton = format_string ("������\n(����)\n��%.0f\n��%.0f", placingZone.cells [0][idxCell].horLen * 1000, placingZone.cells [0][idxCell].verLen * 1000);
						else
							txtButton = format_string ("������\n(����)\n��%.0f\n��%.0f", placingZone.cells [0][idxCell].horLen * 1000, placingZone.cells [0][idxCell].verLen * 1000);
					} else if (placingZone.cells [0][idxCell].objType == FILLERSPACER) {
						txtButton = format_string ("�ٷ�\n��%.0f\n��%.0f", placingZone.cells [0][idxCell].horLen * 1000, placingZone.cells [0][idxCell].verLen * 1000);
					} else if (placingZone.cells [0][idxCell].objType == PLYWOOD) {
						if (placingZone.cells [0][idxCell].libPart.plywood.w_dir_wall)
							txtButton = format_string ("����\n(����)\n��%.0f\n��%.0f", placingZone.cells [0][idxCell].horLen * 1000, placingZone.cells [0][idxCell].verLen * 1000);
						else
							txtButton = format_string ("����\n(����)\n��%.0f\n��%.0f", placingZone.cells [0][idxCell].horLen * 1000, placingZone.cells [0][idxCell].verLen * 1000);
					} else if (placingZone.cells [0][idxCell].objType == WOOD) {
						txtButton = format_string ("����\n��%.0f\n��%.0f", placingZone.cells [0][idxCell].horLen * 1000, placingZone.cells [0][idxCell].verLen * 1000);
					}
					DGSetItemText (dialogID, idxBtn, txtButton.c_str ());		// �׸��� ��ư �ؽ�Ʈ ����
					DGShowItem (dialogID, idxBtn);
					btnPosX += btnSizeX;
				}
				btnPosX = btnInitPosX;
				btnPosY -= btnSizeY;
			}

			break;

		case DG_MSG_CHANGE:

			break;

		case DG_MSG_CLICK:
			switch (item) {
				case PUSHBUTTON_CONFIRM_REMAIN_LENGTH:
					// �������� �ʰ� ���� ���� �Ÿ��� �׸��� ��ư �Ӽ��� ������
					item = 0;

					// �� ����(Ÿ�� �� ũ��) ���� �߻�, ��� ���� ��ġ ���� ������Ʈ
					alignPlacingZoneForWall (&placingZone);
					copyPlacingZoneSymmetricForWall (&placingZone, &placingZoneBackside, &infoWall);

					// ��ư �ε��� iteration �غ�
					idxBtn = itemInitIdx;
					
					// �׸��� ��ư �ؽ�Ʈ ������Ʈ
					for (xx = 0 ; xx < placingZone.eu_count_ver ; ++xx) {
						for (yy = 0 ; yy < placingZone.nCells ; yy += 2) {

							// �� �ε����� ��ư �ε����� ����
							idxCell = ((idxBtn - itemInitIdx) * 2) - (xx * (placingZone.eu_count_hor + 2)) * 2;		// ��ư �ε����� �� �ε����� ����

							txtButton = "";
							if (placingZone.cells [0][yy].objType == NONE) {
								txtButton = "NONE";
							} else if (placingZone.cells [0][yy].objType == INCORNER) {
								txtButton = format_string ("���ڳ�\n��%.0f\n��%.0f", placingZone.cells [0][yy].horLen * 1000, placingZone.cells [0][yy].verLen * 1000);
							} else if (placingZone.cells [0][yy].objType == EUROFORM) {
								if (placingZone.cells [0][yy].libPart.form.u_ins_wall)
									txtButton = format_string ("������\n(����)\n��%.0f\n��%.0f", placingZone.cells [0][yy].horLen * 1000, placingZone.cells [0][yy].verLen * 1000);
								else
									txtButton = format_string ("������\n(����)\n��%.0f\n��%.0f", placingZone.cells [0][yy].horLen * 1000, placingZone.cells [0][yy].verLen * 1000);
							} else if (placingZone.cells [0][yy].objType == FILLERSPACER) {
								txtButton = format_string ("�ٷ�\n��%.0f\n��%.0f", placingZone.cells [0][yy].horLen * 1000, placingZone.cells [0][yy].verLen * 1000);
							} else if (placingZone.cells [0][yy].objType == PLYWOOD) {
								if (placingZone.cells [0][yy].libPart.plywood.w_dir_wall)
									txtButton = format_string ("����\n(����)\n��%.0f\n��%.0f", placingZone.cells [0][yy].horLen * 1000, placingZone.cells [0][yy].verLen * 1000);
								else
									txtButton = format_string ("����\n(����)\n��%.0f\n��%.0f", placingZone.cells [0][yy].horLen * 1000, placingZone.cells [0][yy].verLen * 1000);
							} else if (placingZone.cells [0][yy].objType == WOOD) {
								txtButton = format_string ("����\n��%.0f\n��%.0f", placingZone.cells [0][yy].horLen * 1000, placingZone.cells [0][yy].verLen * 1000);
							}
							DGSetItemText (dialogID, idxBtn, txtButton.c_str ());		// �׸��� ��ư �ؽ�Ʈ ����
							
							// ���� ��ư ���� ���� '����'�� �ƴ϶�� �ش� ���� �۲��� ������
							if ( (idxCell > 0) && (idxCell < (placingZone.nCells - 1)) ) {
								idxCell_prev = idxCell - 1;
								idxCell_next = idxCell + 1;
							} else if (idxCell == 0) {
								idxCell_prev = -1;
								idxCell_next = idxCell + 1;
							} else if (idxCell == (placingZone.nCells - 1)) {
								idxCell_prev = idxCell - 1;
								idxCell_next = -1;
							}

							// ���� ���� ��ü ������ NONE�� �ƴϸ� ��ư �۲� ����
							DGSetItemFont (dialogID, idxBtn, DG_IS_SMALL | DG_IS_PLAIN);
							if (yy == 0) {
								if (placingZone.cells [0][yy+1].objType != NONE)
									DGSetItemFont (dialogID, idxBtn, DG_IS_SMALL | DG_IS_BOLD);
							} else if ( (yy > 0) && (yy < (placingZone.nCells - 2)) ) {
								if ( (placingZone.cells [0][yy-1].objType != NONE) || (placingZone.cells [0][yy+1].objType != NONE) )
									DGSetItemFont (dialogID, idxBtn, DG_IS_SMALL | DG_IS_BOLD);
							} else if ( yy == (placingZone.nCells - 1) ) {
								if (placingZone.cells [0][yy-1].objType != NONE)
									DGSetItemFont (dialogID, idxBtn, DG_IS_SMALL | DG_IS_BOLD);
							}

							++idxBtn;
						}
					}

					// ���� ���� ���� ������Ʈ
					DGSetItemValDouble (dialogID, EDITCONTROL_REMAIN_HORIZONTAL_LENGTH, placingZone.remain_hor_updated);

					break;

				case DG_OK:
					// �������� �ʰ� ��ġ�� ��ü�� ���� �� ���ġ�ϰ� �׸��� ��ư �Ӽ��� ������
					item = 0;

					// �� ����(Ÿ�� �� ũ��) ���� �߻�, ��� ���� ��ġ ���� ������Ʈ
					alignPlacingZoneForWall (&placingZone);
					copyPlacingZoneSymmetricForWall (&placingZone, &placingZoneBackside, &infoWall);

					// ��ư �ε��� iteration �غ�
					idxBtn = itemInitIdx;

					// �׸��� ��ư �ؽ�Ʈ ������Ʈ
					for (xx = 0 ; xx < placingZone.eu_count_ver ; ++xx) {
						for (yy = 0 ; yy < placingZone.nCells ; yy += 2) {

							// �� �ε����� ��ư �ε����� ����
							idxCell = ((idxBtn - itemInitIdx) * 2) - (xx * (placingZone.eu_count_hor + 2)) * 2;		// ��ư �ε����� �� �ε����� ����

							txtButton = "";
							if (placingZone.cells [0][yy].objType == NONE) {
								txtButton = "NONE";
							} else if (placingZone.cells [0][yy].objType == INCORNER) {
								txtButton = format_string ("���ڳ�\n��%.0f\n��%.0f", placingZone.cells [0][yy].horLen * 1000, placingZone.cells [0][yy].verLen * 1000);
							} else if (placingZone.cells [0][yy].objType == EUROFORM) {
								if (placingZone.cells [0][yy].libPart.form.u_ins_wall)
									txtButton = format_string ("������\n(����)\n��%.0f\n��%.0f", placingZone.cells [0][yy].horLen * 1000, placingZone.cells [0][yy].verLen * 1000);
								else
									txtButton = format_string ("������\n(����)\n��%.0f\n��%.0f", placingZone.cells [0][yy].horLen * 1000, placingZone.cells [0][yy].verLen * 1000);
							} else if (placingZone.cells [0][yy].objType == FILLERSPACER) {
								txtButton = format_string ("�ٷ�\n��%.0f\n��%.0f", placingZone.cells [0][yy].horLen * 1000, placingZone.cells [0][yy].verLen * 1000);
							} else if (placingZone.cells [0][yy].objType == PLYWOOD) {
								if (placingZone.cells [0][yy].libPart.plywood.w_dir_wall)
									txtButton = format_string ("����\n(����)\n��%.0f\n��%.0f", placingZone.cells [0][yy].horLen * 1000, placingZone.cells [0][yy].verLen * 1000);
								else
									txtButton = format_string ("����\n(����)\n��%.0f\n��%.0f", placingZone.cells [0][yy].horLen * 1000, placingZone.cells [0][yy].verLen * 1000);
							} else if (placingZone.cells [0][yy].objType == WOOD) {
								txtButton = format_string ("����\n��%.0f\n��%.0f", placingZone.cells [0][yy].horLen * 1000, placingZone.cells [0][yy].verLen * 1000);
							}
							DGSetItemText (dialogID, idxBtn, txtButton.c_str ());		// �׸��� ��ư �ؽ�Ʈ ����

							// ���� ��ư ���� ���� '����'�� �ƴ϶�� �ش� ���� �۲��� ������
							if ( (idxCell > 0) && (idxCell < (placingZone.nCells - 1)) ) {
								idxCell_prev = idxCell - 1;
								idxCell_next = idxCell + 1;
							} else if (idxCell == 0) {
								idxCell_prev = -1;
								idxCell_next = idxCell + 1;
							} else if (idxCell == (placingZone.nCells - 1)) {
								idxCell_prev = idxCell - 1;
								idxCell_next = -1;
							}

							// ���� ���� ��ü ������ NONE�� �ƴϸ� ��ư �۲� ����
							DGSetItemFont (dialogID, idxBtn, DG_IS_SMALL | DG_IS_PLAIN);
							if (yy == 0) {
								if (placingZone.cells [0][yy+1].objType != NONE)
									DGSetItemFont (dialogID, idxBtn, DG_IS_SMALL | DG_IS_BOLD);
							} else if ( (yy > 0) && (yy < (placingZone.nCells - 2)) ) {
								if ( (placingZone.cells [0][yy-1].objType != NONE) || (placingZone.cells [0][yy+1].objType != NONE) )
									DGSetItemFont (dialogID, idxBtn, DG_IS_SMALL | DG_IS_BOLD);
							} else if ( yy == (placingZone.nCells - 1) ) {
								if (placingZone.cells [0][yy-1].objType != NONE)
									DGSetItemFont (dialogID, idxBtn, DG_IS_SMALL | DG_IS_BOLD);
							}

							++idxBtn;
						}
					}

					// ���� ���� ���� ������Ʈ
					DGSetItemValDouble (dialogID, EDITCONTROL_REMAIN_HORIZONTAL_LENGTH, placingZone.remain_hor_updated);

					err = ACAPI_CallUndoableCommand ("������/���ڳ� ���ġ", [&] () -> GSErrCode {
						// ���� ��ġ�� ��ü ���� ����
						for (xx = 0 ; xx < placingZone.eu_count_ver ; ++xx) {
							for (yy = 0 ; yy < placingZone.nCells ; ++ yy) {
								elem.header.guid = placingZone.cells [xx][yy].guid;
								if (ACAPI_Element_Get (&elem) != NoError)
									continue;

								API_Elem_Head* headList = new API_Elem_Head [1];
								headList [0] = elem.header;
								err = ACAPI_Element_Delete (&headList, 1);
								delete headList;
							}
						}

						for (xx = 0 ; xx < placingZoneBackside.eu_count_ver ; ++xx) {
							for (yy = 0 ; yy < placingZoneBackside.nCells ; ++ yy) {
								elem.header.guid = placingZoneBackside.cells [xx][yy].guid;
								if (ACAPI_Element_Get (&elem) != NoError)
									continue;

								API_Elem_Head* headList = new API_Elem_Head [1];
								headList [0] = elem.header;
								err = ACAPI_Element_Delete (&headList, 1);
								delete headList;
							}
						}

						// ������Ʈ�� �� ������� ��ü ���ġ
						//////////////////////////////////////////////////////////// �� ����
						for (xx = 0 ; xx < placingZone.eu_count_ver ; ++xx)
							for (yy = 0 ; yy < placingZone.nCells ; ++yy)
								placingZone.cells [xx][yy].guid = placeLibPart (placingZone.cells [xx][yy]);

						//////////////////////////////////////////////////////////// �� ����
						for (xx = 0 ; xx < placingZoneBackside.eu_count_ver ; ++xx)
							for (yy = 0 ; yy < placingZoneBackside.nCells ; ++yy)
								placingZoneBackside.cells [xx][yy].guid = placeLibPart (placingZoneBackside.cells [xx][yy]);

						return err;
					});

					break;
				case DG_CANCEL:

					break;

				default:
					// [DIALOG] �׸��� ��ư�� ������ Cell�� �����ϱ� ���� ���� â(3��° ���̾�α�)�� ����
					clickedBtnItemIdx = item;
					result = DGBlankModalDialog (240*3, 260, DG_DLG_NOGROW, 0, DG_DLG_NORMALFRAME, wallPlacerHandlerThird, 0);

					item = 0;	// �׸��� ��ư�� ������ �� â�� ������ �ʰ� ��

					break;
			}
		case DG_MSG_CLOSE:
			switch (item) {
				case DG_CLOSEBOX:
					break;
			}
	}

	result = item;

	return	result;
}

// 2�� ���̾�α׿��� �� ���� ��ü Ÿ���� �����ϱ� ���� 3�� ���̾�α�
short DGCALLBACK wallPlacerHandlerThird (short message, short dialogID, short item, DGUserData /* userData */, DGMessageData /* msgData */)
{
	short	result;
	short	idxItem;
	short	idxCell;
	short	idxCell_prev = -1, idxCell_next = -1;
	short	popupSelectedIdx = 0;
	short	xx;
	double	temp;

	switch (message) {
		case DG_MSG_INIT:

			// wallPlacerHandlerSecondary ���� Ŭ���� �׸��� ��ư�� �ε��� ���� �̿��Ͽ� �� �ε��� �� �ε�
			idxCell = (clickedBtnItemIdx - itemInitIdx) * 2;
			while (idxCell >= ((placingZone.eu_count_hor + 2) * 2))
				idxCell -= ((placingZone.eu_count_hor + 2) * 2);

			// ���� ���� �߰� ���̸�,
			if ( (idxCell > 0) && (idxCell < (placingZone.nCells - 1)) ) {
				idxCell_prev = idxCell - 1;
				idxCell_next = idxCell + 1;
			// ���� ���� �� ó�� ���̸�,
			} else if (idxCell == 0) {
				idxCell_prev = -1;
				idxCell_next = idxCell + 1;
			// ���� ���� �� �� ���̸�,
			} else if (idxCell == (placingZone.nCells - 1)) {
				idxCell_prev = idxCell - 1;
				idxCell_next = -1;
			}

			// ���̾�α� Ÿ��Ʋ
			DGSetDialogTitle (dialogID, "Cell �� ����");

			//////////////////////////////////////////////////////////// ������ ��ġ (�⺻ ��ư)
			// ���� ��ư
			DGAppendDialogItem (dialogID, DG_ITM_BUTTON, DG_BT_ICONTEXT, 0, 40+240, 215, 70, 25);
			DGSetItemFont (dialogID, DG_OK, DG_IS_LARGE | DG_IS_PLAIN);
			DGSetItemText (dialogID, DG_OK, "����");
			DGShowItem (dialogID, DG_OK);

			// ���� ��ư
			DGAppendDialogItem (dialogID, DG_ITM_BUTTON, DG_BT_ICONTEXT, 0, 130+240, 215, 70, 25);
			DGSetItemFont (dialogID, DG_CANCEL, DG_IS_LARGE | DG_IS_PLAIN);
			DGSetItemText (dialogID, DG_CANCEL, "���");
			DGShowItem (dialogID, DG_CANCEL);

			//////////////////////////////////////////////////////////// �ʵ� ���� (Ŭ���� ��)
			// ��: ��ü Ÿ��
			idxItem = DGAppendDialogItem (dialogID, DG_ITM_STATICTEXT, DG_IS_RIGHT, DG_FT_NONE, 20+240, 15, 70, 23);
			DGSetItemFont (dialogID, LABEL_OBJ_TYPE, DG_IS_LARGE | DG_IS_PLAIN);
			DGSetItemText (dialogID, LABEL_OBJ_TYPE, "��ü Ÿ��\n[Ŭ���� ��]");
			DGShowItem (dialogID, LABEL_OBJ_TYPE);

			// �˾���Ʈ��: ��ü Ÿ���� �ٲ� �� �ִ� �޺��ڽ��� �� ���� ����
			DGAppendDialogItem (dialogID, DG_ITM_POPUPCONTROL, 25, 5, 100+240, 20-7, 120, 25);
			DGSetItemFont (dialogID, POPUP_OBJ_TYPE, DG_IS_LARGE | DG_IS_PLAIN);
			DGPopUpInsertItem (dialogID, POPUP_OBJ_TYPE, DG_POPUP_BOTTOM);
			DGPopUpSetItemText (dialogID, POPUP_OBJ_TYPE, DG_POPUP_BOTTOM, "����");
			DGPopUpInsertItem (dialogID, POPUP_OBJ_TYPE, DG_POPUP_BOTTOM);
			DGPopUpSetItemText (dialogID, POPUP_OBJ_TYPE, DG_POPUP_BOTTOM, "���ڳ��ǳ�");
			DGPopUpInsertItem (dialogID, POPUP_OBJ_TYPE, DG_POPUP_BOTTOM);
			DGPopUpSetItemText (dialogID, POPUP_OBJ_TYPE, DG_POPUP_BOTTOM, "������");
			DGPopUpInsertItem (dialogID, POPUP_OBJ_TYPE, DG_POPUP_BOTTOM);
			DGPopUpSetItemText (dialogID, POPUP_OBJ_TYPE, DG_POPUP_BOTTOM, "�ٷ������̼�");
			DGPopUpInsertItem (dialogID, POPUP_OBJ_TYPE, DG_POPUP_BOTTOM);
			DGPopUpSetItemText (dialogID, POPUP_OBJ_TYPE, DG_POPUP_BOTTOM, "����");
			DGPopUpInsertItem (dialogID, POPUP_OBJ_TYPE, DG_POPUP_BOTTOM);
			DGPopUpSetItemText (dialogID, POPUP_OBJ_TYPE, DG_POPUP_BOTTOM, "����");
			DGShowItem (dialogID, POPUP_OBJ_TYPE);

			// ��: �ʺ�
			idxItem = DGAppendDialogItem (dialogID, DG_ITM_STATICTEXT, DG_IS_RIGHT, DG_FT_NONE, 20+240, 50, 70, 23);
			DGSetItemFont (dialogID, LABEL_WIDTH, DG_IS_LARGE | DG_IS_PLAIN);
			DGSetItemText (dialogID, LABEL_WIDTH, "�ʺ�");

			// Edit ��Ʈ��: �ʺ�
			DGAppendDialogItem (dialogID, DG_ITM_EDITTEXT, DG_ET_LENGTH, 0, 100+240, 50-6, 50, 25);
			DGSetItemFont (dialogID, EDITCONTROL_WIDTH, DG_IS_LARGE | DG_IS_PLAIN);

			// ��: ����
			idxItem = DGAppendDialogItem (dialogID, DG_ITM_STATICTEXT, DG_IS_RIGHT, DG_FT_NONE, 20+240, 80, 70, 23);
			DGSetItemFont (dialogID, LABEL_HEIGHT, DG_IS_LARGE | DG_IS_PLAIN);
			DGSetItemText (dialogID, LABEL_HEIGHT, "����");

			// Edit ��Ʈ��: ����
			DGAppendDialogItem (dialogID, DG_ITM_EDITTEXT, DG_ET_LENGTH, 0, 100+240, 80-6, 50, 25);
			DGSetItemFont (dialogID, EDITCONTROL_HEIGHT, DG_IS_LARGE | DG_IS_PLAIN);

			// ��: �β�
			idxItem = DGAppendDialogItem (dialogID, DG_ITM_STATICTEXT, DG_IS_RIGHT, DG_FT_NONE, 20+240, 110, 70, 23);
			DGSetItemFont (dialogID, LABEL_THK, DG_IS_LARGE | DG_IS_PLAIN);
			DGSetItemText (dialogID, LABEL_THK, "�β�");

			// Edit ��Ʈ��: �β�
			DGAppendDialogItem (dialogID, DG_ITM_EDITTEXT, DG_ET_LENGTH, 0, 100+240, 110-6, 50, 25);
			DGSetItemFont (dialogID, EDITCONTROL_THK, DG_IS_LARGE | DG_IS_PLAIN);

			// ��: ��ġ����
			idxItem = DGAppendDialogItem (dialogID, DG_ITM_STATICTEXT, DG_IS_RIGHT, DG_FT_NONE, 20+240, 140, 70, 23);
			DGSetItemFont (dialogID, LABEL_ORIENTATION, DG_IS_LARGE | DG_IS_PLAIN);
			DGSetItemText (dialogID, LABEL_ORIENTATION, "��ġ����");
				
			// ���� ��ư: ��ġ���� (�������)
			idxItem = DGAppendDialogItem (dialogID, DG_ITM_RADIOBUTTON, DG_BT_PUSHTEXT, 777, 100+240, 140-6, 70, 25);
			DGSetItemFont (dialogID, RADIO_ORIENTATION_1_PLYWOOD, DG_IS_LARGE | DG_IS_PLAIN);
			DGSetItemText (dialogID, RADIO_ORIENTATION_1_PLYWOOD, "�������");
			// ���� ��ư: ��ġ���� (��������)
			idxItem = DGAppendDialogItem (dialogID, DG_ITM_RADIOBUTTON, DG_BT_PUSHTEXT, 777, 100+240, 170-6, 70, 25);
			DGSetItemFont (dialogID, RADIO_ORIENTATION_2_PLYWOOD, DG_IS_LARGE | DG_IS_PLAIN);
			DGSetItemText (dialogID, RADIO_ORIENTATION_2_PLYWOOD, "��������");

			// üũ�ڽ�: �԰���
			DGAppendDialogItem (dialogID, DG_ITM_CHECKBOX, DG_BT_TEXT, 0, 20+240, 50, 70, 25-5);
			DGSetItemFont (dialogID, CHECKBOX_SET_STANDARD, DG_IS_LARGE | DG_IS_PLAIN);
			DGSetItemText (dialogID, CHECKBOX_SET_STANDARD, "�԰���");

			// ��: �ʺ�
			idxItem = DGAppendDialogItem (dialogID, DG_ITM_STATICTEXT, DG_IS_RIGHT, DG_FT_NONE, 20+240, 80, 70, 23);
			DGSetItemFont (dialogID, LABEL_EUROFORM_WIDTH_OPTIONS, DG_IS_LARGE | DG_IS_PLAIN);
			DGSetItemText (dialogID, LABEL_EUROFORM_WIDTH_OPTIONS, "�ʺ�");

			// �˾� ��Ʈ��: �ʺ�
			DGAppendDialogItem (dialogID, DG_ITM_POPUPCONTROL, 25, 5, 100+240, 80-7, 100, 25);
			DGSetItemFont (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS, DG_IS_LARGE | DG_IS_PLAIN);
			DGPopUpInsertItem (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS, DG_POPUP_BOTTOM);
			DGPopUpSetItemText (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS, DG_POPUP_BOTTOM, "600");
			DGPopUpInsertItem (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS, DG_POPUP_BOTTOM);
			DGPopUpSetItemText (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS, DG_POPUP_BOTTOM, "500");
			DGPopUpInsertItem (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS, DG_POPUP_BOTTOM);
			DGPopUpSetItemText (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS, DG_POPUP_BOTTOM, "450");
			DGPopUpInsertItem (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS, DG_POPUP_BOTTOM);
			DGPopUpSetItemText (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS, DG_POPUP_BOTTOM, "400");
			DGPopUpInsertItem (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS, DG_POPUP_BOTTOM);
			DGPopUpSetItemText (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS, DG_POPUP_BOTTOM, "300");
			DGPopUpInsertItem (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS, DG_POPUP_BOTTOM);
			DGPopUpSetItemText (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS, DG_POPUP_BOTTOM, "200");

			// Edit ��Ʈ��: �ʺ�
			DGAppendDialogItem (dialogID, DG_ITM_EDITTEXT, DG_ET_LENGTH, 0, 100+240, 80-6, 50, 25);
			DGSetItemFont (dialogID, EDITCONTROL_EUROFORM_WIDTH_OPTIONS, DG_IS_LARGE | DG_IS_PLAIN);

			// ��: ����
			idxItem = DGAppendDialogItem (dialogID, DG_ITM_STATICTEXT, DG_IS_RIGHT, DG_FT_NONE, 20+240, 110, 70, 23);
			DGSetItemFont (dialogID, LABEL_EUROFORM_HEIGHT_OPTIONS, DG_IS_LARGE | DG_IS_PLAIN);
			DGSetItemText (dialogID, LABEL_EUROFORM_HEIGHT_OPTIONS, "����");

			// �˾� ��Ʈ��: ����
			DGAppendDialogItem (dialogID, DG_ITM_POPUPCONTROL, 25, 5, 100+240, 110-7, 100, 25);
			DGSetItemFont (dialogID, POPUP_EUROFORM_HEIGHT_OPTIONS, DG_IS_LARGE | DG_IS_PLAIN);
			DGPopUpInsertItem (dialogID, POPUP_EUROFORM_HEIGHT_OPTIONS, DG_POPUP_BOTTOM);
			DGPopUpSetItemText (dialogID, POPUP_EUROFORM_HEIGHT_OPTIONS, DG_POPUP_BOTTOM, "1200");
			DGPopUpInsertItem (dialogID, POPUP_EUROFORM_HEIGHT_OPTIONS, DG_POPUP_BOTTOM);
			DGPopUpSetItemText (dialogID, POPUP_EUROFORM_HEIGHT_OPTIONS, DG_POPUP_BOTTOM, "900");
			DGPopUpInsertItem (dialogID, POPUP_EUROFORM_HEIGHT_OPTIONS, DG_POPUP_BOTTOM);
			DGPopUpSetItemText (dialogID, POPUP_EUROFORM_HEIGHT_OPTIONS, DG_POPUP_BOTTOM, "600");

			// Edit ��Ʈ��: ����
			DGAppendDialogItem (dialogID, DG_ITM_EDITTEXT, DG_ET_LENGTH, 0, 100+240, 110-6, 50, 25);
			DGSetItemFont (dialogID, EDITCONTROL_EUROFORM_HEIGHT_OPTIONS, DG_IS_LARGE | DG_IS_PLAIN);
			
			// ��: ��ġ����
			idxItem = DGAppendDialogItem (dialogID, DG_ITM_STATICTEXT, DG_IS_RIGHT, DG_FT_NONE, 20+240, 140, 70, 23);
			DGSetItemFont (dialogID, LABEL_EUROFORM_ORIENTATION_OPTIONS, DG_IS_LARGE | DG_IS_PLAIN);
			DGSetItemText (dialogID, LABEL_EUROFORM_ORIENTATION_OPTIONS, "��ġ����");
			
			// ���� ��ư: ��ġ���� (�������)
			idxItem = DGAppendDialogItem (dialogID, DG_ITM_RADIOBUTTON, DG_BT_PUSHTEXT, 778, 100+240, 140-6, 70, 25);
			DGSetItemFont (dialogID, RADIO_ORIENTATION_1_EUROFORM, DG_IS_LARGE | DG_IS_PLAIN);
			DGSetItemText (dialogID, RADIO_ORIENTATION_1_EUROFORM, "�������");
			// ���� ��ư: ��ġ���� (��������)
			idxItem = DGAppendDialogItem (dialogID, DG_ITM_RADIOBUTTON, DG_BT_PUSHTEXT, 778, 100+240, 170-6, 70, 25);
			DGSetItemFont (dialogID, RADIO_ORIENTATION_2_EUROFORM, DG_IS_LARGE | DG_IS_PLAIN);
			DGSetItemText (dialogID, RADIO_ORIENTATION_2_EUROFORM, "��������");

			// �ʱ� �Է� �ʵ� ǥ��
			if (placingZone.cells [0][idxCell].objType == INCORNER) {
				DGPopUpSelectItem (dialogID, POPUP_OBJ_TYPE, INCORNER + 1);

				// ��: �ʺ�
				DGShowItem (dialogID, LABEL_WIDTH);

				// Edit ��Ʈ��: �ʺ�
				DGShowItem (dialogID, EDITCONTROL_WIDTH);
				if (idxCell == 0)
					DGSetItemValDouble (dialogID, EDITCONTROL_WIDTH, placingZone.cells [0][idxCell].libPart.incorner.leng_s);
				else if (idxCell > 0)
					DGSetItemValDouble (dialogID, EDITCONTROL_WIDTH, placingZone.cells [0][idxCell].libPart.incorner.wid_s);
				DGSetItemMinDouble (dialogID, EDITCONTROL_WIDTH, 0.080);
				DGSetItemMaxDouble (dialogID, EDITCONTROL_WIDTH, 0.500);

				// �� ����
				DGShowItem (dialogID, LABEL_HEIGHT);

				// Edit ��Ʈ��: ����
				DGShowItem (dialogID, EDITCONTROL_HEIGHT);
				DGSetItemValDouble (dialogID, EDITCONTROL_HEIGHT, placingZone.cells [0][idxCell].libPart.incorner.hei_s);
				DGSetItemMinDouble (dialogID, EDITCONTROL_HEIGHT, 0.050);
				DGSetItemMaxDouble (dialogID, EDITCONTROL_HEIGHT, 1.500);

			} else if (placingZone.cells [0][idxCell].objType == EUROFORM) {
				DGPopUpSelectItem (dialogID, POPUP_OBJ_TYPE, EUROFORM + 1);

				// üũ�ڽ�: �԰���
				DGShowItem (dialogID, CHECKBOX_SET_STANDARD);
				DGSetItemValLong (dialogID, CHECKBOX_SET_STANDARD, placingZone.cells [0][idxCell].libPart.form.eu_stan_onoff);

				if (placingZone.cells [0][idxCell].libPart.form.eu_stan_onoff == true) {
					// ��: �ʺ�
					DGShowItem (dialogID, LABEL_EUROFORM_WIDTH_OPTIONS);

					// �˾� ��Ʈ��: �ʺ�
					DGShowItem (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS);
					if (abs(placingZone.cells [0][idxCell].libPart.form.eu_wid - 0.600) < EPS)		popupSelectedIdx = 1;
					if (abs(placingZone.cells [0][idxCell].libPart.form.eu_wid - 0.500) < EPS)		popupSelectedIdx = 2;
					if (abs(placingZone.cells [0][idxCell].libPart.form.eu_wid - 0.450) < EPS)		popupSelectedIdx = 3;
					if (abs(placingZone.cells [0][idxCell].libPart.form.eu_wid - 0.400) < EPS)		popupSelectedIdx = 4;
					if (abs(placingZone.cells [0][idxCell].libPart.form.eu_wid - 0.300) < EPS)		popupSelectedIdx = 5;
					if (abs(placingZone.cells [0][idxCell].libPart.form.eu_wid - 0.200) < EPS)		popupSelectedIdx = 6;
					DGPopUpSelectItem (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS, popupSelectedIdx);

					// ��: ����
					DGShowItem (dialogID, LABEL_EUROFORM_HEIGHT_OPTIONS);

					// �˾� ��Ʈ��: ����
					DGShowItem (dialogID, POPUP_EUROFORM_HEIGHT_OPTIONS);
					if (abs(placingZone.cells [0][idxCell].libPart.form.eu_hei - 1.200) < EPS)		popupSelectedIdx = 1;
					if (abs(placingZone.cells [0][idxCell].libPart.form.eu_hei - 0.900) < EPS)		popupSelectedIdx = 2;
					if (abs(placingZone.cells [0][idxCell].libPart.form.eu_hei - 0.600) < EPS)		popupSelectedIdx = 3;
					DGPopUpSelectItem (dialogID, POPUP_EUROFORM_HEIGHT_OPTIONS, popupSelectedIdx);
				} else if (placingZone.cells [0][idxCell].libPart.form.eu_stan_onoff == false) {
					// ��: �ʺ�
					DGShowItem (dialogID, LABEL_EUROFORM_WIDTH_OPTIONS);

					// Edit ��Ʈ��: �ʺ�
					DGShowItem (dialogID, EDITCONTROL_EUROFORM_WIDTH_OPTIONS);
					DGSetItemValDouble (dialogID, EDITCONTROL_EUROFORM_WIDTH_OPTIONS, placingZone.cells [0][idxCell].libPart.form.eu_wid2);
					DGSetItemMinDouble (dialogID, EDITCONTROL_EUROFORM_WIDTH_OPTIONS, 0.050);
					DGSetItemMaxDouble (dialogID, EDITCONTROL_EUROFORM_WIDTH_OPTIONS, 0.900);

					// ��: ����
					DGShowItem (dialogID, LABEL_EUROFORM_HEIGHT_OPTIONS);

					// Edit ��Ʈ��: ����
					DGShowItem (dialogID, EDITCONTROL_EUROFORM_HEIGHT_OPTIONS);
					DGSetItemValDouble (dialogID, EDITCONTROL_EUROFORM_HEIGHT_OPTIONS, placingZone.cells [0][idxCell].libPart.form.eu_hei2);
					DGSetItemMinDouble (dialogID, EDITCONTROL_EUROFORM_HEIGHT_OPTIONS, 0.050);
					DGSetItemMaxDouble (dialogID, EDITCONTROL_EUROFORM_HEIGHT_OPTIONS, 1.500);
				}

				// ��: ��ġ����
				DGShowItem (dialogID, LABEL_EUROFORM_ORIENTATION_OPTIONS);
				
				// ���� ��ư: ��ġ���� (�������)
				DGShowItem (dialogID, RADIO_ORIENTATION_1_EUROFORM);
				// ���� ��ư: ��ġ���� (��������)
				DGShowItem (dialogID, RADIO_ORIENTATION_2_EUROFORM);

				if (placingZone.cells [0][idxCell].libPart.form.u_ins_wall == true) {
					DGSetItemValLong (dialogID, RADIO_ORIENTATION_1_EUROFORM, true);
					DGSetItemValLong (dialogID, RADIO_ORIENTATION_2_EUROFORM, false);
				} else if (placingZone.cells [0][idxCell].libPart.form.u_ins_wall == false) {
					DGSetItemValLong (dialogID, RADIO_ORIENTATION_1_EUROFORM, false);
					DGSetItemValLong (dialogID, RADIO_ORIENTATION_2_EUROFORM, true);
				}
			} else if (placingZone.cells [0][idxCell].objType == FILLERSPACER) {
				DGPopUpSelectItem (dialogID, POPUP_OBJ_TYPE, FILLERSPACER + 1);

				// ��: �ʺ�
				DGShowItem (dialogID, LABEL_WIDTH);

				// Edit ��Ʈ��: �ʺ�
				DGShowItem (dialogID, EDITCONTROL_WIDTH);
				DGSetItemValDouble (dialogID, EDITCONTROL_WIDTH, placingZone.cells [0][idxCell].libPart.fillersp.f_thk);
				DGSetItemMinDouble (dialogID, EDITCONTROL_WIDTH, 0.010);
				DGSetItemMaxDouble (dialogID, EDITCONTROL_WIDTH, 0.050);

				// �� ����
				DGShowItem (dialogID, LABEL_HEIGHT);

				// Edit ��Ʈ��: ����
				DGShowItem (dialogID, EDITCONTROL_HEIGHT);
				DGSetItemValDouble (dialogID, EDITCONTROL_HEIGHT, placingZone.cells [0][idxCell].libPart.fillersp.f_leng);
				DGSetItemMinDouble (dialogID, EDITCONTROL_HEIGHT, 0.150);
				DGSetItemMaxDouble (dialogID, EDITCONTROL_HEIGHT, 2.400);

			} else if (placingZone.cells [0][idxCell].objType == PLYWOOD) {
				DGPopUpSelectItem (dialogID, POPUP_OBJ_TYPE, PLYWOOD + 1);

				// ��: �ʺ�
				DGShowItem (dialogID, LABEL_WIDTH);

				// Edit ��Ʈ��: �ʺ�
				DGShowItem (dialogID, EDITCONTROL_WIDTH);
				DGSetItemValDouble (dialogID, EDITCONTROL_WIDTH, placingZone.cells [0][idxCell].libPart.plywood.p_wid);
				DGSetItemMinDouble (dialogID, EDITCONTROL_WIDTH, 0.110);
				DGSetItemMaxDouble (dialogID, EDITCONTROL_WIDTH, 1.220);

				// ��: ����
				DGShowItem (dialogID, LABEL_HEIGHT);

				// Edit ��Ʈ��: ����
				DGShowItem (dialogID, EDITCONTROL_HEIGHT);
				DGSetItemValDouble (dialogID, EDITCONTROL_HEIGHT, placingZone.cells [0][idxCell].libPart.plywood.p_leng);
				DGSetItemMinDouble (dialogID, EDITCONTROL_HEIGHT, 0.110);
				DGSetItemMaxDouble (dialogID, EDITCONTROL_HEIGHT, 2.440);

				// ��: ��ġ����
				DGShowItem (dialogID, LABEL_ORIENTATION);
				
				// ���� ��ư: ��ġ���� (�������)
				DGShowItem (dialogID, RADIO_ORIENTATION_1_PLYWOOD);
				// ���� ��ư: ��ġ���� (��������)
				DGShowItem (dialogID, RADIO_ORIENTATION_2_PLYWOOD);

				if (placingZone.cells [0][idxCell].libPart.plywood.w_dir_wall == true) {
					DGSetItemValLong (dialogID, RADIO_ORIENTATION_1_PLYWOOD, true);
					DGSetItemValLong (dialogID, RADIO_ORIENTATION_2_PLYWOOD, false);
				} else if (placingZone.cells [0][idxCell].libPart.plywood.w_dir_wall == false) {
					DGSetItemValLong (dialogID, RADIO_ORIENTATION_1_PLYWOOD, false);
					DGSetItemValLong (dialogID, RADIO_ORIENTATION_2_PLYWOOD, true);
				}
			} else if (placingZone.cells [0][idxCell].objType == WOOD) {
				DGPopUpSelectItem (dialogID, POPUP_OBJ_TYPE, WOOD + 1);

				// ��: �ʺ�
				DGShowItem (dialogID, LABEL_WIDTH);

				// Edit ��Ʈ��: �ʺ�
				DGShowItem (dialogID, EDITCONTROL_WIDTH);
				DGSetItemValDouble (dialogID, EDITCONTROL_WIDTH, placingZone.cells [0][idxCell].libPart.wood.w_h);
				DGSetItemMinDouble (dialogID, EDITCONTROL_WIDTH, 0.005);
				DGSetItemMaxDouble (dialogID, EDITCONTROL_WIDTH, 1.000);

				// ��: ����
				DGShowItem (dialogID, LABEL_HEIGHT);

				// Edit ��Ʈ��: ����
				DGShowItem (dialogID, EDITCONTROL_HEIGHT);
				DGSetItemValDouble (dialogID, EDITCONTROL_HEIGHT, placingZone.cells [0][idxCell].libPart.wood.w_leng);
				DGSetItemMinDouble (dialogID, EDITCONTROL_HEIGHT, 0.010);
				DGSetItemMaxDouble (dialogID, EDITCONTROL_HEIGHT, 3.600);

				// ��: �β�
				DGShowItem (dialogID, LABEL_THK);

				// Edit ��Ʈ��: �β�
				DGShowItem (dialogID, EDITCONTROL_THK);
				DGSetItemValDouble (dialogID, EDITCONTROL_THK, placingZone.cells [0][idxCell].libPart.wood.w_w);
				DGSetItemMinDouble (dialogID, EDITCONTROL_THK, 0.005);
				DGSetItemMaxDouble (dialogID, EDITCONTROL_THK, 1.000);
			}

			//////////////////////////////////////////////////////////// �ʵ� ���� (���� ��)
			// ��: ��ü Ÿ��
			idxItem = DGAppendDialogItem (dialogID, DG_ITM_STATICTEXT, DG_IS_RIGHT, DG_FT_NONE, 20+0, 15, 70, 23);
			DGSetItemFont (dialogID, LABEL_OBJ_TYPE_PREV, DG_IS_LARGE | DG_IS_PLAIN);
			DGSetItemText (dialogID, LABEL_OBJ_TYPE_PREV, "��ü Ÿ��\n[���� ��]");
			if (idxCell_prev != -1)	DGShowItem (dialogID, LABEL_OBJ_TYPE_PREV);

			// �˾���Ʈ��: ��ü Ÿ���� �ٲ� �� �ִ� �޺��ڽ��� �� ���� ����
			DGAppendDialogItem (dialogID, DG_ITM_POPUPCONTROL, 25, 5, 100+0, 20-7, 120, 25);
			DGSetItemFont (dialogID, POPUP_OBJ_TYPE_PREV, DG_IS_LARGE | DG_IS_PLAIN);
			DGPopUpInsertItem (dialogID, POPUP_OBJ_TYPE_PREV, DG_POPUP_BOTTOM);
			DGPopUpSetItemText (dialogID, POPUP_OBJ_TYPE_PREV, DG_POPUP_BOTTOM, "����");
			DGPopUpInsertItem (dialogID, POPUP_OBJ_TYPE_PREV, DG_POPUP_BOTTOM);
			DGPopUpSetItemText (dialogID, POPUP_OBJ_TYPE_PREV, DG_POPUP_BOTTOM, "���ڳ��ǳ�");
			DGPopUpInsertItem (dialogID, POPUP_OBJ_TYPE_PREV, DG_POPUP_BOTTOM);
			DGPopUpSetItemText (dialogID, POPUP_OBJ_TYPE_PREV, DG_POPUP_BOTTOM, "������");
			DGPopUpInsertItem (dialogID, POPUP_OBJ_TYPE_PREV, DG_POPUP_BOTTOM);
			DGPopUpSetItemText (dialogID, POPUP_OBJ_TYPE_PREV, DG_POPUP_BOTTOM, "�ٷ������̼�");
			DGPopUpInsertItem (dialogID, POPUP_OBJ_TYPE_PREV, DG_POPUP_BOTTOM);
			DGPopUpSetItemText (dialogID, POPUP_OBJ_TYPE_PREV, DG_POPUP_BOTTOM, "����");
			DGPopUpInsertItem (dialogID, POPUP_OBJ_TYPE_PREV, DG_POPUP_BOTTOM);
			DGPopUpSetItemText (dialogID, POPUP_OBJ_TYPE_PREV, DG_POPUP_BOTTOM, "����");
			if (idxCell_prev != -1)	DGShowItem (dialogID, POPUP_OBJ_TYPE_PREV);

			// ��: �ʺ�
			idxItem = DGAppendDialogItem (dialogID, DG_ITM_STATICTEXT, DG_IS_RIGHT, DG_FT_NONE, 20+0, 50, 70, 23);
			DGSetItemFont (dialogID, LABEL_WIDTH_PREV, DG_IS_LARGE | DG_IS_PLAIN);
			DGSetItemText (dialogID, LABEL_WIDTH_PREV, "�ʺ�");

			// Edit ��Ʈ��: �ʺ�
			DGAppendDialogItem (dialogID, DG_ITM_EDITTEXT, DG_ET_LENGTH, 0, 100+0, 50-6, 50, 25);
			DGSetItemFont (dialogID, EDITCONTROL_WIDTH_PREV, DG_IS_LARGE | DG_IS_PLAIN);

			// ��: ����
			idxItem = DGAppendDialogItem (dialogID, DG_ITM_STATICTEXT, DG_IS_RIGHT, DG_FT_NONE, 20+0, 80, 70, 23);
			DGSetItemFont (dialogID, LABEL_HEIGHT_PREV, DG_IS_LARGE | DG_IS_PLAIN);
			DGSetItemText (dialogID, LABEL_HEIGHT_PREV, "����");

			// Edit ��Ʈ��: ����
			DGAppendDialogItem (dialogID, DG_ITM_EDITTEXT, DG_ET_LENGTH, 0, 100+0, 80-6, 50, 25);
			DGSetItemFont (dialogID, EDITCONTROL_HEIGHT_PREV, DG_IS_LARGE | DG_IS_PLAIN);

			// ��: �β�
			idxItem = DGAppendDialogItem (dialogID, DG_ITM_STATICTEXT, DG_IS_RIGHT, DG_FT_NONE, 20+0, 110, 70, 23);
			DGSetItemFont (dialogID, LABEL_THK_PREV, DG_IS_LARGE | DG_IS_PLAIN);
			DGSetItemText (dialogID, LABEL_THK_PREV, "�β�");

			// Edit ��Ʈ��: �β�
			DGAppendDialogItem (dialogID, DG_ITM_EDITTEXT, DG_ET_LENGTH, 0, 100+0, 110-6, 50, 25);
			DGSetItemFont (dialogID, EDITCONTROL_THK_PREV, DG_IS_LARGE | DG_IS_PLAIN);
			
			// ��: ��ġ����
			idxItem = DGAppendDialogItem (dialogID, DG_ITM_STATICTEXT, DG_IS_RIGHT, DG_FT_NONE, 20+0, 140, 70, 23);
			DGSetItemFont (dialogID, LABEL_ORIENTATION_PREV, DG_IS_LARGE | DG_IS_PLAIN);
			DGSetItemText (dialogID, LABEL_ORIENTATION_PREV, "��ġ����");
				
			// ���� ��ư: ��ġ���� (�������)
			idxItem = DGAppendDialogItem (dialogID, DG_ITM_RADIOBUTTON, DG_BT_PUSHTEXT, 779, 100+0, 140-6, 70, 25);
			DGSetItemFont (dialogID, RADIO_ORIENTATION_1_PLYWOOD_PREV, DG_IS_LARGE | DG_IS_PLAIN);
			DGSetItemText (dialogID, RADIO_ORIENTATION_1_PLYWOOD_PREV, "�������");
			// ���� ��ư: ��ġ���� (��������)
			idxItem = DGAppendDialogItem (dialogID, DG_ITM_RADIOBUTTON, DG_BT_PUSHTEXT, 779, 100+0, 170-6, 70, 25);
			DGSetItemFont (dialogID, RADIO_ORIENTATION_2_PLYWOOD_PREV, DG_IS_LARGE | DG_IS_PLAIN);
			DGSetItemText (dialogID, RADIO_ORIENTATION_2_PLYWOOD_PREV, "��������");

			// üũ�ڽ�: �԰���
			DGAppendDialogItem (dialogID, DG_ITM_CHECKBOX, DG_BT_TEXT, 0, 20+0, 50, 70, 25-5);
			DGSetItemFont (dialogID, CHECKBOX_SET_STANDARD_PREV, DG_IS_LARGE | DG_IS_PLAIN);
			DGSetItemText (dialogID, CHECKBOX_SET_STANDARD_PREV, "�԰���");

			// ��: �ʺ�
			idxItem = DGAppendDialogItem (dialogID, DG_ITM_STATICTEXT, DG_IS_RIGHT, DG_FT_NONE, 20+0, 80, 70, 23);
			DGSetItemFont (dialogID, LABEL_EUROFORM_WIDTH_OPTIONS_PREV, DG_IS_LARGE | DG_IS_PLAIN);
			DGSetItemText (dialogID, LABEL_EUROFORM_WIDTH_OPTIONS_PREV, "�ʺ�");

			// �˾� ��Ʈ��: �ʺ�
			DGAppendDialogItem (dialogID, DG_ITM_POPUPCONTROL, 25, 5, 100+0, 80-7, 100, 25);
			DGSetItemFont (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS_PREV, DG_IS_LARGE | DG_IS_PLAIN);
			DGPopUpInsertItem (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS_PREV, DG_POPUP_BOTTOM);
			DGPopUpSetItemText (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS_PREV, DG_POPUP_BOTTOM, "600");
			DGPopUpInsertItem (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS_PREV, DG_POPUP_BOTTOM);
			DGPopUpSetItemText (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS_PREV, DG_POPUP_BOTTOM, "500");
			DGPopUpInsertItem (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS_PREV, DG_POPUP_BOTTOM);
			DGPopUpSetItemText (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS_PREV, DG_POPUP_BOTTOM, "450");
			DGPopUpInsertItem (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS_PREV, DG_POPUP_BOTTOM);
			DGPopUpSetItemText (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS_PREV, DG_POPUP_BOTTOM, "400");
			DGPopUpInsertItem (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS_PREV, DG_POPUP_BOTTOM);
			DGPopUpSetItemText (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS_PREV, DG_POPUP_BOTTOM, "300");
			DGPopUpInsertItem (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS_PREV, DG_POPUP_BOTTOM);
			DGPopUpSetItemText (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS_PREV, DG_POPUP_BOTTOM, "200");

			// Edit ��Ʈ��: �ʺ�
			DGAppendDialogItem (dialogID, DG_ITM_EDITTEXT, DG_ET_LENGTH, 0, 100+0, 80-6, 50, 25);
			DGSetItemFont (dialogID, EDITCONTROL_EUROFORM_WIDTH_OPTIONS_PREV, DG_IS_LARGE | DG_IS_PLAIN);

			// ��: ����
			idxItem = DGAppendDialogItem (dialogID, DG_ITM_STATICTEXT, DG_IS_RIGHT, DG_FT_NONE, 20+0, 110, 70, 23);
			DGSetItemFont (dialogID, LABEL_EUROFORM_HEIGHT_OPTIONS_PREV, DG_IS_LARGE | DG_IS_PLAIN);
			DGSetItemText (dialogID, LABEL_EUROFORM_HEIGHT_OPTIONS_PREV, "����");

			// �˾� ��Ʈ��: ����
			DGAppendDialogItem (dialogID, DG_ITM_POPUPCONTROL, 25, 5, 100+0, 110-7, 100, 25);
			DGSetItemFont (dialogID, POPUP_EUROFORM_HEIGHT_OPTIONS_PREV, DG_IS_LARGE | DG_IS_PLAIN);
			DGPopUpInsertItem (dialogID, POPUP_EUROFORM_HEIGHT_OPTIONS_PREV, DG_POPUP_BOTTOM);
			DGPopUpSetItemText (dialogID, POPUP_EUROFORM_HEIGHT_OPTIONS_PREV, DG_POPUP_BOTTOM, "1200");
			DGPopUpInsertItem (dialogID, POPUP_EUROFORM_HEIGHT_OPTIONS_PREV, DG_POPUP_BOTTOM);
			DGPopUpSetItemText (dialogID, POPUP_EUROFORM_HEIGHT_OPTIONS_PREV, DG_POPUP_BOTTOM, "900");
			DGPopUpInsertItem (dialogID, POPUP_EUROFORM_HEIGHT_OPTIONS_PREV, DG_POPUP_BOTTOM);
			DGPopUpSetItemText (dialogID, POPUP_EUROFORM_HEIGHT_OPTIONS_PREV, DG_POPUP_BOTTOM, "600");

			// Edit ��Ʈ��: ����
			DGAppendDialogItem (dialogID, DG_ITM_EDITTEXT, DG_ET_LENGTH, 0, 100+0, 110-6, 50, 25);
			DGSetItemFont (dialogID, EDITCONTROL_EUROFORM_HEIGHT_OPTIONS_PREV, DG_IS_LARGE | DG_IS_PLAIN);
			
			// ��: ��ġ����
			idxItem = DGAppendDialogItem (dialogID, DG_ITM_STATICTEXT, DG_IS_RIGHT, DG_FT_NONE, 20+0, 140, 70, 23);
			DGSetItemFont (dialogID, LABEL_EUROFORM_ORIENTATION_OPTIONS_PREV, DG_IS_LARGE | DG_IS_PLAIN);
			DGSetItemText (dialogID, LABEL_EUROFORM_ORIENTATION_OPTIONS_PREV, "��ġ����");
			
			// ���� ��ư: ��ġ���� (�������)
			idxItem = DGAppendDialogItem (dialogID, DG_ITM_RADIOBUTTON, DG_BT_PUSHTEXT, 780, 100+0, 140-6, 70, 25);
			DGSetItemFont (dialogID, RADIO_ORIENTATION_1_EUROFORM_PREV, DG_IS_LARGE | DG_IS_PLAIN);
			DGSetItemText (dialogID, RADIO_ORIENTATION_1_EUROFORM_PREV, "�������");
			// ���� ��ư: ��ġ���� (��������)
			idxItem = DGAppendDialogItem (dialogID, DG_ITM_RADIOBUTTON, DG_BT_PUSHTEXT, 780, 100+0, 170-6, 70, 25);
			DGSetItemFont (dialogID, RADIO_ORIENTATION_2_EUROFORM_PREV, DG_IS_LARGE | DG_IS_PLAIN);
			DGSetItemText (dialogID, RADIO_ORIENTATION_2_EUROFORM_PREV, "��������");

			// �ʱ� �Է� �ʵ� ǥ��
			if (idxCell_prev != -1) {
				if (placingZone.cells [0][idxCell_prev].objType == INCORNER) {
					DGPopUpSelectItem (dialogID, POPUP_OBJ_TYPE_PREV, INCORNER + 1);

					// ��: �ʺ�
					DGShowItem (dialogID, LABEL_WIDTH_PREV);

					// Edit ��Ʈ��: �ʺ�
					DGShowItem (dialogID, EDITCONTROL_WIDTH_PREV);
					if (idxCell_prev == 0)
						DGSetItemValDouble (dialogID, EDITCONTROL_WIDTH_PREV, placingZone.cells [0][idxCell_prev].libPart.incorner.leng_s);
					else if (idxCell_prev > 0)
						DGSetItemValDouble (dialogID, EDITCONTROL_WIDTH_PREV, placingZone.cells [0][idxCell_prev].libPart.incorner.wid_s);
					DGSetItemMinDouble (dialogID, EDITCONTROL_WIDTH_PREV, 0.080);
					DGSetItemMaxDouble (dialogID, EDITCONTROL_WIDTH_PREV, 0.500);

					// �� ����
					DGShowItem (dialogID, LABEL_HEIGHT_PREV);

					// Edit ��Ʈ��: ����
					DGShowItem (dialogID, EDITCONTROL_HEIGHT_PREV);
					DGSetItemValDouble (dialogID, EDITCONTROL_HEIGHT_PREV, placingZone.cells [0][idxCell_prev].libPart.incorner.hei_s);
					DGSetItemMinDouble (dialogID, EDITCONTROL_HEIGHT_PREV, 0.050);
					DGSetItemMaxDouble (dialogID, EDITCONTROL_HEIGHT_PREV, 1.500);

				} else if (placingZone.cells [0][idxCell_prev].objType == EUROFORM) {
					DGPopUpSelectItem (dialogID, POPUP_OBJ_TYPE_PREV, EUROFORM + 1);

					// üũ�ڽ�: �԰���
					DGShowItem (dialogID, CHECKBOX_SET_STANDARD_PREV);
					DGSetItemValLong (dialogID, CHECKBOX_SET_STANDARD_PREV, placingZone.cells [0][idxCell_prev].libPart.form.eu_stan_onoff);

					if (placingZone.cells [0][idxCell_prev].libPart.form.eu_stan_onoff == true) {
						// ��: �ʺ�
						DGShowItem (dialogID, LABEL_EUROFORM_WIDTH_OPTIONS_PREV);

						// �˾� ��Ʈ��: �ʺ�
						DGShowItem (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS_PREV);
						if (abs(placingZone.cells [0][idxCell_prev].libPart.form.eu_wid - 0.600) < EPS)		popupSelectedIdx = 1;
						if (abs(placingZone.cells [0][idxCell_prev].libPart.form.eu_wid - 0.500) < EPS)		popupSelectedIdx = 2;
						if (abs(placingZone.cells [0][idxCell_prev].libPart.form.eu_wid - 0.450) < EPS)		popupSelectedIdx = 3;
						if (abs(placingZone.cells [0][idxCell_prev].libPart.form.eu_wid - 0.400) < EPS)		popupSelectedIdx = 4;
						if (abs(placingZone.cells [0][idxCell_prev].libPart.form.eu_wid - 0.300) < EPS)		popupSelectedIdx = 5;
						if (abs(placingZone.cells [0][idxCell_prev].libPart.form.eu_wid - 0.200) < EPS)		popupSelectedIdx = 6;
						DGPopUpSelectItem (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS_PREV, popupSelectedIdx);

						// ��: ����
						DGShowItem (dialogID, LABEL_EUROFORM_HEIGHT_OPTIONS_PREV);

						// �˾� ��Ʈ��: ����
						DGShowItem (dialogID, POPUP_EUROFORM_HEIGHT_OPTIONS_PREV);
						if (abs(placingZone.cells [0][idxCell_prev].libPart.form.eu_hei - 1.200) < EPS)		popupSelectedIdx = 1;
						if (abs(placingZone.cells [0][idxCell_prev].libPart.form.eu_hei - 0.900) < EPS)		popupSelectedIdx = 2;
						if (abs(placingZone.cells [0][idxCell_prev].libPart.form.eu_hei - 0.600) < EPS)		popupSelectedIdx = 3;
						DGPopUpSelectItem (dialogID, POPUP_EUROFORM_HEIGHT_OPTIONS_PREV, popupSelectedIdx);
					} else if (placingZone.cells [0][idxCell_prev].libPart.form.eu_stan_onoff == false) {
						// ��: �ʺ�
						DGShowItem (dialogID, LABEL_EUROFORM_WIDTH_OPTIONS_PREV);

						// Edit ��Ʈ��: �ʺ�
						DGShowItem (dialogID, EDITCONTROL_EUROFORM_WIDTH_OPTIONS_PREV);
						DGSetItemValDouble (dialogID, EDITCONTROL_EUROFORM_WIDTH_OPTIONS_PREV, placingZone.cells [0][idxCell_prev].libPart.form.eu_wid2);
						DGSetItemMinDouble (dialogID, EDITCONTROL_EUROFORM_WIDTH_OPTIONS_PREV, 0.050);
						DGSetItemMaxDouble (dialogID, EDITCONTROL_EUROFORM_WIDTH_OPTIONS_PREV, 0.900);

						// ��: ����
						DGShowItem (dialogID, LABEL_EUROFORM_HEIGHT_OPTIONS_PREV);

						// Edit ��Ʈ��: ����
						DGShowItem (dialogID, EDITCONTROL_EUROFORM_HEIGHT_OPTIONS_PREV);
						DGSetItemValDouble (dialogID, EDITCONTROL_EUROFORM_HEIGHT_OPTIONS_PREV, placingZone.cells [0][idxCell_prev].libPart.form.eu_hei2);
						DGSetItemMinDouble (dialogID, EDITCONTROL_EUROFORM_HEIGHT_OPTIONS_PREV, 0.050);
						DGSetItemMaxDouble (dialogID, EDITCONTROL_EUROFORM_HEIGHT_OPTIONS_PREV, 1.500);
					}

					// ��: ��ġ����
					DGShowItem (dialogID, LABEL_EUROFORM_ORIENTATION_OPTIONS_PREV);
				
					// ���� ��ư: ��ġ���� (�������)
					DGShowItem (dialogID, RADIO_ORIENTATION_1_EUROFORM_PREV);
					// ���� ��ư: ��ġ���� (��������)
					DGShowItem (dialogID, RADIO_ORIENTATION_2_EUROFORM_PREV);

					if (placingZone.cells [0][idxCell_prev].libPart.form.u_ins_wall == true) {
						DGSetItemValLong (dialogID, RADIO_ORIENTATION_1_EUROFORM_PREV, true);
						DGSetItemValLong (dialogID, RADIO_ORIENTATION_2_EUROFORM_PREV, false);
					} else if (placingZone.cells [0][idxCell_prev].libPart.form.u_ins_wall == false) {
						DGSetItemValLong (dialogID, RADIO_ORIENTATION_1_EUROFORM_PREV, false);
						DGSetItemValLong (dialogID, RADIO_ORIENTATION_2_EUROFORM_PREV, true);
					}
				} else if (placingZone.cells [0][idxCell_prev].objType == FILLERSPACER) {
					DGPopUpSelectItem (dialogID, POPUP_OBJ_TYPE_PREV, FILLERSPACER + 1);

					// ��: �ʺ�
					DGShowItem (dialogID, LABEL_WIDTH_PREV);

					// Edit ��Ʈ��: �ʺ�
					DGShowItem (dialogID, EDITCONTROL_WIDTH_PREV);
					DGSetItemValDouble (dialogID, EDITCONTROL_WIDTH_PREV, placingZone.cells [0][idxCell_prev].libPart.fillersp.f_thk);
					DGSetItemMinDouble (dialogID, EDITCONTROL_WIDTH_PREV, 0.010);
					DGSetItemMaxDouble (dialogID, EDITCONTROL_WIDTH_PREV, 0.050);

					// �� ����
					DGShowItem (dialogID, LABEL_HEIGHT_PREV);

					// Edit ��Ʈ��: ����
					DGShowItem (dialogID, EDITCONTROL_HEIGHT_PREV);
					DGSetItemValDouble (dialogID, EDITCONTROL_HEIGHT_PREV, placingZone.cells [0][idxCell_prev].libPart.fillersp.f_leng);
					DGSetItemMinDouble (dialogID, EDITCONTROL_HEIGHT_PREV, 0.150);
					DGSetItemMaxDouble (dialogID, EDITCONTROL_HEIGHT_PREV, 2.400);

				} else if (placingZone.cells [0][idxCell_prev].objType == PLYWOOD) {
					DGPopUpSelectItem (dialogID, POPUP_OBJ_TYPE_PREV, PLYWOOD + 1);

					// ��: �ʺ�
					DGShowItem (dialogID, LABEL_WIDTH_PREV);

					// Edit ��Ʈ��: �ʺ�
					DGShowItem (dialogID, EDITCONTROL_WIDTH_PREV);
					DGSetItemValDouble (dialogID, EDITCONTROL_WIDTH_PREV, placingZone.cells [0][idxCell_prev].libPart.plywood.p_wid);
					DGSetItemMinDouble (dialogID, EDITCONTROL_WIDTH_PREV, 0.110);
					DGSetItemMaxDouble (dialogID, EDITCONTROL_WIDTH_PREV, 1.220);

					// ��: ����
					DGShowItem (dialogID, LABEL_HEIGHT_PREV);

					// Edit ��Ʈ��: ����
					DGShowItem (dialogID, EDITCONTROL_HEIGHT_PREV);
					DGSetItemValDouble (dialogID, EDITCONTROL_HEIGHT_PREV, placingZone.cells [0][idxCell_prev].libPart.plywood.p_leng);
					DGSetItemMinDouble (dialogID, EDITCONTROL_HEIGHT_PREV, 0.110);
					DGSetItemMaxDouble (dialogID, EDITCONTROL_HEIGHT_PREV, 2.440);

					// ��: ��ġ����
					DGShowItem (dialogID, LABEL_ORIENTATION_PREV);
				
					// ���� ��ư: ��ġ���� (�������)
					DGShowItem (dialogID, RADIO_ORIENTATION_1_PLYWOOD_PREV);
					// ���� ��ư: ��ġ���� (��������)
					DGShowItem (dialogID, RADIO_ORIENTATION_2_PLYWOOD_PREV);

					if (placingZone.cells [0][idxCell_prev].libPart.plywood.w_dir_wall == true) {
						DGSetItemValLong (dialogID, RADIO_ORIENTATION_1_PLYWOOD_PREV, true);
						DGSetItemValLong (dialogID, RADIO_ORIENTATION_2_PLYWOOD_PREV, false);
					} else if (placingZone.cells [0][idxCell_prev].libPart.plywood.w_dir_wall == false) {
						DGSetItemValLong (dialogID, RADIO_ORIENTATION_1_PLYWOOD_PREV, false);
						DGSetItemValLong (dialogID, RADIO_ORIENTATION_2_PLYWOOD_PREV, true);
					}
				} else if (placingZone.cells [0][idxCell_prev].objType == WOOD) {
					DGPopUpSelectItem (dialogID, POPUP_OBJ_TYPE_PREV, WOOD + 1);

					// ��: �ʺ�
					DGShowItem (dialogID, LABEL_WIDTH_PREV);

					// Edit ��Ʈ��: �ʺ�
					DGShowItem (dialogID, EDITCONTROL_WIDTH_PREV);
					DGSetItemValDouble (dialogID, EDITCONTROL_WIDTH_PREV, placingZone.cells [0][idxCell_prev].libPart.wood.w_h);
					DGSetItemMinDouble (dialogID, EDITCONTROL_WIDTH_PREV, 0.005);
					DGSetItemMaxDouble (dialogID, EDITCONTROL_WIDTH_PREV, 1.000);

					// ��: ����
					DGShowItem (dialogID, LABEL_HEIGHT_PREV);

					// Edit ��Ʈ��: ����
					DGShowItem (dialogID, EDITCONTROL_HEIGHT_PREV);
					DGSetItemValDouble (dialogID, EDITCONTROL_HEIGHT_PREV, placingZone.cells [0][idxCell_prev].libPart.wood.w_leng);
					DGSetItemMinDouble (dialogID, EDITCONTROL_HEIGHT_PREV, 0.010);
					DGSetItemMaxDouble (dialogID, EDITCONTROL_HEIGHT_PREV, 3.600);

					// ��: �β�
					DGShowItem (dialogID, LABEL_THK_PREV);

					// Edit ��Ʈ��: �β�
					DGShowItem (dialogID, EDITCONTROL_THK_PREV);
					DGSetItemValDouble (dialogID, EDITCONTROL_THK_PREV, placingZone.cells [0][idxCell_prev].libPart.wood.w_w);
					DGSetItemMinDouble (dialogID, EDITCONTROL_THK_PREV, 0.005);
					DGSetItemMaxDouble (dialogID, EDITCONTROL_THK_PREV, 1.000);
				}
			}

			//////////////////////////////////////////////////////////// �ʵ� ���� (���� ��)
			// ��: ��ü Ÿ��
			idxItem = DGAppendDialogItem (dialogID, DG_ITM_STATICTEXT, DG_IS_RIGHT, DG_FT_NONE, 20+480, 15, 70, 23);
			DGSetItemFont (dialogID, LABEL_OBJ_TYPE_NEXT, DG_IS_LARGE | DG_IS_PLAIN);
			DGSetItemText (dialogID, LABEL_OBJ_TYPE_NEXT, "��ü Ÿ��\n[���� ��]");
			if (idxCell_next != -1)	DGShowItem (dialogID, LABEL_OBJ_TYPE_NEXT);

			// �˾���Ʈ��: ��ü Ÿ���� �ٲ� �� �ִ� �޺��ڽ��� �� ���� ����
			DGAppendDialogItem (dialogID, DG_ITM_POPUPCONTROL, 25, 5, 100+480, 20-7, 120, 25);
			DGSetItemFont (dialogID, POPUP_OBJ_TYPE_NEXT, DG_IS_LARGE | DG_IS_PLAIN);
			DGPopUpInsertItem (dialogID, POPUP_OBJ_TYPE_NEXT, DG_POPUP_BOTTOM);
			DGPopUpSetItemText (dialogID, POPUP_OBJ_TYPE_NEXT, DG_POPUP_BOTTOM, "����");
			DGPopUpInsertItem (dialogID, POPUP_OBJ_TYPE_NEXT, DG_POPUP_BOTTOM);
			DGPopUpSetItemText (dialogID, POPUP_OBJ_TYPE_NEXT, DG_POPUP_BOTTOM, "���ڳ��ǳ�");
			DGPopUpInsertItem (dialogID, POPUP_OBJ_TYPE_NEXT, DG_POPUP_BOTTOM);
			DGPopUpSetItemText (dialogID, POPUP_OBJ_TYPE_NEXT, DG_POPUP_BOTTOM, "������");
			DGPopUpInsertItem (dialogID, POPUP_OBJ_TYPE_NEXT, DG_POPUP_BOTTOM);
			DGPopUpSetItemText (dialogID, POPUP_OBJ_TYPE_NEXT, DG_POPUP_BOTTOM, "�ٷ������̼�");
			DGPopUpInsertItem (dialogID, POPUP_OBJ_TYPE_NEXT, DG_POPUP_BOTTOM);
			DGPopUpSetItemText (dialogID, POPUP_OBJ_TYPE_NEXT, DG_POPUP_BOTTOM, "����");
			DGPopUpInsertItem (dialogID, POPUP_OBJ_TYPE_NEXT, DG_POPUP_BOTTOM);
			DGPopUpSetItemText (dialogID, POPUP_OBJ_TYPE_NEXT, DG_POPUP_BOTTOM, "����");
			if (idxCell_next != -1)	DGShowItem (dialogID, POPUP_OBJ_TYPE_NEXT);

			// ��: �ʺ�
			idxItem = DGAppendDialogItem (dialogID, DG_ITM_STATICTEXT, DG_IS_RIGHT, DG_FT_NONE, 20+480, 50, 70, 23);
			DGSetItemFont (dialogID, LABEL_WIDTH_NEXT, DG_IS_LARGE | DG_IS_PLAIN);
			DGSetItemText (dialogID, LABEL_WIDTH_NEXT, "�ʺ�");

			// Edit ��Ʈ��: �ʺ�
			DGAppendDialogItem (dialogID, DG_ITM_EDITTEXT, DG_ET_LENGTH, 0, 100+480, 50-6, 50, 25);
			DGSetItemFont (dialogID, EDITCONTROL_WIDTH_NEXT, DG_IS_LARGE | DG_IS_PLAIN);

			// ��: ����
			idxItem = DGAppendDialogItem (dialogID, DG_ITM_STATICTEXT, DG_IS_RIGHT, DG_FT_NONE, 20+480, 80, 70, 23);
			DGSetItemFont (dialogID, LABEL_HEIGHT_NEXT, DG_IS_LARGE | DG_IS_PLAIN);
			DGSetItemText (dialogID, LABEL_HEIGHT_NEXT, "����");

			// Edit ��Ʈ��: ����
			DGAppendDialogItem (dialogID, DG_ITM_EDITTEXT, DG_ET_LENGTH, 0, 100+480, 80-6, 50, 25);
			DGSetItemFont (dialogID, EDITCONTROL_HEIGHT_NEXT, DG_IS_LARGE | DG_IS_PLAIN);
			
			// ��: �β�
			idxItem = DGAppendDialogItem (dialogID, DG_ITM_STATICTEXT, DG_IS_RIGHT, DG_FT_NONE, 20+480, 110, 70, 23);
			DGSetItemFont (dialogID, LABEL_THK_NEXT, DG_IS_LARGE | DG_IS_PLAIN);
			DGSetItemText (dialogID, LABEL_THK_NEXT, "�β�");

			// Edit ��Ʈ��: �β�
			DGAppendDialogItem (dialogID, DG_ITM_EDITTEXT, DG_ET_LENGTH, 0, 100+480, 110-6, 50, 25);
			DGSetItemFont (dialogID, EDITCONTROL_THK_NEXT, DG_IS_LARGE | DG_IS_PLAIN);

			// ��: ��ġ����
			idxItem = DGAppendDialogItem (dialogID, DG_ITM_STATICTEXT, DG_IS_RIGHT, DG_FT_NONE, 20+480, 140, 70, 23);
			DGSetItemFont (dialogID, LABEL_ORIENTATION_NEXT, DG_IS_LARGE | DG_IS_PLAIN);
			DGSetItemText (dialogID, LABEL_ORIENTATION_NEXT, "��ġ����");
				
			// ���� ��ư: ��ġ���� (�������)
			idxItem = DGAppendDialogItem (dialogID, DG_ITM_RADIOBUTTON, DG_BT_PUSHTEXT, 781, 100+480, 140-6, 70, 25);
			DGSetItemFont (dialogID, RADIO_ORIENTATION_1_PLYWOOD_NEXT, DG_IS_LARGE | DG_IS_PLAIN);
			DGSetItemText (dialogID, RADIO_ORIENTATION_1_PLYWOOD_NEXT, "�������");
			// ���� ��ư: ��ġ���� (��������)
			idxItem = DGAppendDialogItem (dialogID, DG_ITM_RADIOBUTTON, DG_BT_PUSHTEXT, 781, 100+480, 170-6, 70, 25);
			DGSetItemFont (dialogID, RADIO_ORIENTATION_2_PLYWOOD_NEXT, DG_IS_LARGE | DG_IS_PLAIN);
			DGSetItemText (dialogID, RADIO_ORIENTATION_2_PLYWOOD_NEXT, "��������");

			// üũ�ڽ�: �԰���
			DGAppendDialogItem (dialogID, DG_ITM_CHECKBOX, DG_BT_TEXT, 0, 20+480, 50, 70, 25-5);
			DGSetItemFont (dialogID, CHECKBOX_SET_STANDARD_NEXT, DG_IS_LARGE | DG_IS_PLAIN);
			DGSetItemText (dialogID, CHECKBOX_SET_STANDARD_NEXT, "�԰���");

			// ��: �ʺ�
			idxItem = DGAppendDialogItem (dialogID, DG_ITM_STATICTEXT, DG_IS_RIGHT, DG_FT_NONE, 20+480, 80, 70, 23);
			DGSetItemFont (dialogID, LABEL_EUROFORM_WIDTH_OPTIONS_NEXT, DG_IS_LARGE | DG_IS_PLAIN);
			DGSetItemText (dialogID, LABEL_EUROFORM_WIDTH_OPTIONS_NEXT, "�ʺ�");

			// �˾� ��Ʈ��: �ʺ�
			DGAppendDialogItem (dialogID, DG_ITM_POPUPCONTROL, 25, 5, 100+480, 80-7, 100, 25);
			DGSetItemFont (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS_NEXT, DG_IS_LARGE | DG_IS_PLAIN);
			DGPopUpInsertItem (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS_NEXT, DG_POPUP_BOTTOM);
			DGPopUpSetItemText (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS_NEXT, DG_POPUP_BOTTOM, "600");
			DGPopUpInsertItem (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS_NEXT, DG_POPUP_BOTTOM);
			DGPopUpSetItemText (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS_NEXT, DG_POPUP_BOTTOM, "500");
			DGPopUpInsertItem (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS_NEXT, DG_POPUP_BOTTOM);
			DGPopUpSetItemText (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS_NEXT, DG_POPUP_BOTTOM, "450");
			DGPopUpInsertItem (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS_NEXT, DG_POPUP_BOTTOM);
			DGPopUpSetItemText (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS_NEXT, DG_POPUP_BOTTOM, "400");
			DGPopUpInsertItem (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS_NEXT, DG_POPUP_BOTTOM);
			DGPopUpSetItemText (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS_NEXT, DG_POPUP_BOTTOM, "300");
			DGPopUpInsertItem (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS_NEXT, DG_POPUP_BOTTOM);
			DGPopUpSetItemText (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS_NEXT, DG_POPUP_BOTTOM, "200");

			// Edit ��Ʈ��: �ʺ�
			DGAppendDialogItem (dialogID, DG_ITM_EDITTEXT, DG_ET_LENGTH, 0, 100+480, 80-6, 50, 25);
			DGSetItemFont (dialogID, EDITCONTROL_EUROFORM_WIDTH_OPTIONS_NEXT, DG_IS_LARGE | DG_IS_PLAIN);

			// ��: ����
			idxItem = DGAppendDialogItem (dialogID, DG_ITM_STATICTEXT, DG_IS_RIGHT, DG_FT_NONE, 20+480, 110, 70, 23);
			DGSetItemFont (dialogID, LABEL_EUROFORM_HEIGHT_OPTIONS_NEXT, DG_IS_LARGE | DG_IS_PLAIN);
			DGSetItemText (dialogID, LABEL_EUROFORM_HEIGHT_OPTIONS_NEXT, "����");

			// �˾� ��Ʈ��: ����
			DGAppendDialogItem (dialogID, DG_ITM_POPUPCONTROL, 25, 5, 100+480, 110-7, 100, 25);
			DGSetItemFont (dialogID, POPUP_EUROFORM_HEIGHT_OPTIONS_NEXT, DG_IS_LARGE | DG_IS_PLAIN);
			DGPopUpInsertItem (dialogID, POPUP_EUROFORM_HEIGHT_OPTIONS_NEXT, DG_POPUP_BOTTOM);
			DGPopUpSetItemText (dialogID, POPUP_EUROFORM_HEIGHT_OPTIONS_NEXT, DG_POPUP_BOTTOM, "1200");
			DGPopUpInsertItem (dialogID, POPUP_EUROFORM_HEIGHT_OPTIONS_NEXT, DG_POPUP_BOTTOM);
			DGPopUpSetItemText (dialogID, POPUP_EUROFORM_HEIGHT_OPTIONS_NEXT, DG_POPUP_BOTTOM, "900");
			DGPopUpInsertItem (dialogID, POPUP_EUROFORM_HEIGHT_OPTIONS_NEXT, DG_POPUP_BOTTOM);
			DGPopUpSetItemText (dialogID, POPUP_EUROFORM_HEIGHT_OPTIONS_NEXT, DG_POPUP_BOTTOM, "600");

			// Edit ��Ʈ��: ����
			DGAppendDialogItem (dialogID, DG_ITM_EDITTEXT, DG_ET_LENGTH, 0, 100+480, 110-6, 50, 25);
			DGSetItemFont (dialogID, EDITCONTROL_EUROFORM_HEIGHT_OPTIONS_NEXT, DG_IS_LARGE | DG_IS_PLAIN);
			
			// ��: ��ġ����
			idxItem = DGAppendDialogItem (dialogID, DG_ITM_STATICTEXT, DG_IS_RIGHT, DG_FT_NONE, 20+480, 140, 70, 23);
			DGSetItemFont (dialogID, LABEL_EUROFORM_ORIENTATION_OPTIONS_NEXT, DG_IS_LARGE | DG_IS_PLAIN);
			DGSetItemText (dialogID, LABEL_EUROFORM_ORIENTATION_OPTIONS_NEXT, "��ġ����");
			
			// ���� ��ư: ��ġ���� (�������)
			idxItem = DGAppendDialogItem (dialogID, DG_ITM_RADIOBUTTON, DG_BT_PUSHTEXT, 782, 100+480, 140-6, 70, 25);
			DGSetItemFont (dialogID, RADIO_ORIENTATION_1_EUROFORM_NEXT, DG_IS_LARGE | DG_IS_PLAIN);
			DGSetItemText (dialogID, RADIO_ORIENTATION_1_EUROFORM_NEXT, "�������");
			// ���� ��ư: ��ġ���� (��������)
			idxItem = DGAppendDialogItem (dialogID, DG_ITM_RADIOBUTTON, DG_BT_PUSHTEXT, 782, 100+480, 170-6, 70, 25);
			DGSetItemFont (dialogID, RADIO_ORIENTATION_2_EUROFORM_NEXT, DG_IS_LARGE | DG_IS_PLAIN);
			DGSetItemText (dialogID, RADIO_ORIENTATION_2_EUROFORM_NEXT, "��������");

			// �ʱ� �Է� �ʵ� ǥ��
			if (idxCell_next != -1) {
				if (placingZone.cells [0][idxCell_next].objType == INCORNER) {
					DGPopUpSelectItem (dialogID, POPUP_OBJ_TYPE_NEXT, INCORNER + 1);

					// ��: �ʺ�
					DGShowItem (dialogID, LABEL_WIDTH_NEXT);

					// Edit ��Ʈ��: �ʺ�
					DGShowItem (dialogID, EDITCONTROL_WIDTH_NEXT);
					if (idxCell_next == 0)
						DGSetItemValDouble (dialogID, EDITCONTROL_WIDTH_NEXT, placingZone.cells [0][idxCell_next].libPart.incorner.leng_s);
					else if (idxCell_next > 0)
						DGSetItemValDouble (dialogID, EDITCONTROL_WIDTH_NEXT, placingZone.cells [0][idxCell_next].libPart.incorner.wid_s);
					DGSetItemMinDouble (dialogID, EDITCONTROL_WIDTH_NEXT, 0.080);
					DGSetItemMaxDouble (dialogID, EDITCONTROL_WIDTH_NEXT, 0.500);

					// �� ����
					DGShowItem (dialogID, LABEL_HEIGHT_NEXT);

					// Edit ��Ʈ��: ����
					DGShowItem (dialogID, EDITCONTROL_HEIGHT_NEXT);
					DGSetItemValDouble (dialogID, EDITCONTROL_HEIGHT_NEXT, placingZone.cells [0][idxCell_next].libPart.incorner.hei_s);
					DGSetItemMinDouble (dialogID, EDITCONTROL_HEIGHT_NEXT, 0.050);
					DGSetItemMaxDouble (dialogID, EDITCONTROL_HEIGHT_NEXT, 1.500);

				} else if (placingZone.cells [0][idxCell_next].objType == EUROFORM) {
					DGPopUpSelectItem (dialogID, POPUP_OBJ_TYPE_NEXT, EUROFORM + 1);

					// üũ�ڽ�: �԰���
					DGShowItem (dialogID, CHECKBOX_SET_STANDARD_NEXT);
					DGSetItemValLong (dialogID, CHECKBOX_SET_STANDARD_NEXT, placingZone.cells [0][idxCell_next].libPart.form.eu_stan_onoff);

					if (placingZone.cells [0][idxCell_next].libPart.form.eu_stan_onoff == true) {
						// ��: �ʺ�
						DGShowItem (dialogID, LABEL_EUROFORM_WIDTH_OPTIONS_NEXT);

						// �˾� ��Ʈ��: �ʺ�
						DGShowItem (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS_NEXT);
						if (abs(placingZone.cells [0][idxCell_next].libPart.form.eu_wid - 0.600) < EPS)		popupSelectedIdx = 1;
						if (abs(placingZone.cells [0][idxCell_next].libPart.form.eu_wid - 0.500) < EPS)		popupSelectedIdx = 2;
						if (abs(placingZone.cells [0][idxCell_next].libPart.form.eu_wid - 0.450) < EPS)		popupSelectedIdx = 3;
						if (abs(placingZone.cells [0][idxCell_next].libPart.form.eu_wid - 0.400) < EPS)		popupSelectedIdx = 4;
						if (abs(placingZone.cells [0][idxCell_next].libPart.form.eu_wid - 0.300) < EPS)		popupSelectedIdx = 5;
						if (abs(placingZone.cells [0][idxCell_next].libPart.form.eu_wid - 0.200) < EPS)		popupSelectedIdx = 6;
						DGPopUpSelectItem (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS_NEXT, popupSelectedIdx);

						// ��: ����
						DGShowItem (dialogID, LABEL_EUROFORM_HEIGHT_OPTIONS_NEXT);

						// �˾� ��Ʈ��: ����
						DGShowItem (dialogID, POPUP_EUROFORM_HEIGHT_OPTIONS_NEXT);
						if (abs(placingZone.cells [0][idxCell_next].libPart.form.eu_hei - 1.200) < EPS)		popupSelectedIdx = 1;
						if (abs(placingZone.cells [0][idxCell_next].libPart.form.eu_hei - 0.900) < EPS)		popupSelectedIdx = 2;
						if (abs(placingZone.cells [0][idxCell_next].libPart.form.eu_hei - 0.600) < EPS)		popupSelectedIdx = 3;
						DGPopUpSelectItem (dialogID, POPUP_EUROFORM_HEIGHT_OPTIONS_NEXT, popupSelectedIdx);
					} else if (placingZone.cells [0][idxCell_next].libPart.form.eu_stan_onoff == false) {
						// ��: �ʺ�
						DGShowItem (dialogID, LABEL_EUROFORM_WIDTH_OPTIONS_NEXT);

						// Edit ��Ʈ��: �ʺ�
						DGShowItem (dialogID, EDITCONTROL_EUROFORM_WIDTH_OPTIONS_NEXT);
						DGSetItemValDouble (dialogID, EDITCONTROL_EUROFORM_WIDTH_OPTIONS_NEXT, placingZone.cells [0][idxCell_next].libPart.form.eu_wid2);
						DGSetItemMinDouble (dialogID, EDITCONTROL_EUROFORM_WIDTH_OPTIONS_NEXT, 0.050);
						DGSetItemMaxDouble (dialogID, EDITCONTROL_EUROFORM_WIDTH_OPTIONS_NEXT, 0.900);

						// ��: ����
						DGShowItem (dialogID, LABEL_EUROFORM_HEIGHT_OPTIONS_NEXT);

						// Edit ��Ʈ��: ����
						DGShowItem (dialogID, EDITCONTROL_EUROFORM_HEIGHT_OPTIONS_NEXT);
						DGSetItemValDouble (dialogID, EDITCONTROL_EUROFORM_HEIGHT_OPTIONS_NEXT, placingZone.cells [0][idxCell_next].libPart.form.eu_hei2);
						DGSetItemMinDouble (dialogID, EDITCONTROL_EUROFORM_HEIGHT_OPTIONS_NEXT, 0.050);
						DGSetItemMaxDouble (dialogID, EDITCONTROL_EUROFORM_HEIGHT_OPTIONS_NEXT, 1.500);
					}

					// ��: ��ġ����
					DGShowItem (dialogID, LABEL_EUROFORM_ORIENTATION_OPTIONS_NEXT);
				
					// ���� ��ư: ��ġ���� (�������)
					DGShowItem (dialogID, RADIO_ORIENTATION_1_EUROFORM_NEXT);
					// ���� ��ư: ��ġ���� (��������)
					DGShowItem (dialogID, RADIO_ORIENTATION_2_EUROFORM_NEXT);

					if (placingZone.cells [0][idxCell_next].libPart.form.u_ins_wall == true) {
						DGSetItemValLong (dialogID, RADIO_ORIENTATION_1_EUROFORM_NEXT, true);
						DGSetItemValLong (dialogID, RADIO_ORIENTATION_2_EUROFORM_NEXT, false);
					} else if (placingZone.cells [0][idxCell_next].libPart.form.u_ins_wall == false) {
						DGSetItemValLong (dialogID, RADIO_ORIENTATION_1_EUROFORM_NEXT, false);
						DGSetItemValLong (dialogID, RADIO_ORIENTATION_2_EUROFORM_NEXT, true);
					}
				} else if (placingZone.cells [0][idxCell_next].objType == FILLERSPACER) {
					DGPopUpSelectItem (dialogID, POPUP_OBJ_TYPE_NEXT, FILLERSPACER + 1);

					// ��: �ʺ�
					DGShowItem (dialogID, LABEL_WIDTH_NEXT);

					// Edit ��Ʈ��: �ʺ�
					DGShowItem (dialogID, EDITCONTROL_WIDTH_NEXT);
					DGSetItemValDouble (dialogID, EDITCONTROL_WIDTH_NEXT, placingZone.cells [0][idxCell_next].libPart.fillersp.f_thk);
					DGSetItemMinDouble (dialogID, EDITCONTROL_WIDTH_NEXT, 0.010);
					DGSetItemMaxDouble (dialogID, EDITCONTROL_WIDTH_NEXT, 0.050);

					// �� ����
					DGShowItem (dialogID, LABEL_HEIGHT_NEXT);

					// Edit ��Ʈ��: ����
					DGShowItem (dialogID, EDITCONTROL_HEIGHT_NEXT);
					DGSetItemValDouble (dialogID, EDITCONTROL_HEIGHT_NEXT, placingZone.cells [0][idxCell_next].libPart.fillersp.f_leng);
					DGSetItemMinDouble (dialogID, EDITCONTROL_HEIGHT_NEXT, 0.150);
					DGSetItemMaxDouble (dialogID, EDITCONTROL_HEIGHT_NEXT, 2.400);

				} else if (placingZone.cells [0][idxCell_next].objType == PLYWOOD) {
					DGPopUpSelectItem (dialogID, POPUP_OBJ_TYPE_NEXT, PLYWOOD + 1);

					// ��: �ʺ�
					DGShowItem (dialogID, LABEL_WIDTH_NEXT);

					// Edit ��Ʈ��: �ʺ�
					DGShowItem (dialogID, EDITCONTROL_WIDTH_NEXT);
					DGSetItemValDouble (dialogID, EDITCONTROL_WIDTH_NEXT, placingZone.cells [0][idxCell_next].libPart.plywood.p_wid);
					DGSetItemMinDouble (dialogID, EDITCONTROL_WIDTH_NEXT, 0.110);
					DGSetItemMaxDouble (dialogID, EDITCONTROL_WIDTH_NEXT, 1.220);

					// ��: ����
					DGShowItem (dialogID, LABEL_HEIGHT_NEXT);

					// Edit ��Ʈ��: ����
					DGShowItem (dialogID, EDITCONTROL_HEIGHT_NEXT);
					DGSetItemValDouble (dialogID, EDITCONTROL_HEIGHT_NEXT, placingZone.cells [0][idxCell_next].libPart.plywood.p_leng);
					DGSetItemMinDouble (dialogID, EDITCONTROL_HEIGHT_NEXT, 0.110);
					DGSetItemMaxDouble (dialogID, EDITCONTROL_HEIGHT_NEXT, 2.440);

					// ��: ��ġ����
					DGShowItem (dialogID, LABEL_ORIENTATION_NEXT);
				
					// ���� ��ư: ��ġ���� (�������)
					DGShowItem (dialogID, RADIO_ORIENTATION_1_PLYWOOD_NEXT);
					// ���� ��ư: ��ġ���� (��������)
					DGShowItem (dialogID, RADIO_ORIENTATION_2_PLYWOOD_NEXT);

					if (placingZone.cells [0][idxCell_next].libPart.plywood.w_dir_wall == true) {
						DGSetItemValLong (dialogID, RADIO_ORIENTATION_1_PLYWOOD_NEXT, true);
						DGSetItemValLong (dialogID, RADIO_ORIENTATION_2_PLYWOOD_NEXT, false);
					} else if (placingZone.cells [0][idxCell_next].libPart.plywood.w_dir_wall == false) {
						DGSetItemValLong (dialogID, RADIO_ORIENTATION_1_PLYWOOD_NEXT, false);
						DGSetItemValLong (dialogID, RADIO_ORIENTATION_2_PLYWOOD_NEXT, true);
					}
				} else if (placingZone.cells [0][idxCell_next].objType == WOOD) {
					DGPopUpSelectItem (dialogID, POPUP_OBJ_TYPE_NEXT, WOOD + 1);

					// ��: �ʺ�
					DGShowItem (dialogID, LABEL_WIDTH_NEXT);

					// Edit ��Ʈ��: �ʺ�
					DGShowItem (dialogID, EDITCONTROL_WIDTH_NEXT);
					DGSetItemValDouble (dialogID, EDITCONTROL_WIDTH_NEXT, placingZone.cells [0][idxCell_next].libPart.wood.w_h);
					DGSetItemMinDouble (dialogID, EDITCONTROL_WIDTH_NEXT, 0.005);
					DGSetItemMaxDouble (dialogID, EDITCONTROL_WIDTH_NEXT, 1.000);

					// ��: ����
					DGShowItem (dialogID, LABEL_HEIGHT_NEXT);

					// Edit ��Ʈ��: ����
					DGShowItem (dialogID, EDITCONTROL_HEIGHT_NEXT);
					DGSetItemValDouble (dialogID, EDITCONTROL_HEIGHT_NEXT, placingZone.cells [0][idxCell_next].libPart.wood.w_leng);
					DGSetItemMinDouble (dialogID, EDITCONTROL_HEIGHT_NEXT, 0.010);
					DGSetItemMaxDouble (dialogID, EDITCONTROL_HEIGHT_NEXT, 3.600);

					// ��: �β�
					DGShowItem (dialogID, LABEL_HEIGHT_NEXT);

					// Edit ��Ʈ��: �β�
					DGShowItem (dialogID, EDITCONTROL_THK_NEXT);
					DGSetItemValDouble (dialogID, EDITCONTROL_THK_NEXT, placingZone.cells [0][idxCell_next].libPart.wood.w_w);
					DGSetItemMinDouble (dialogID, EDITCONTROL_THK_NEXT, 0.005);
					DGSetItemMaxDouble (dialogID, EDITCONTROL_THK_NEXT, 1.000);
				}
			}

			break;

		case DG_MSG_CHANGE:
			switch (item) {
				case POPUP_OBJ_TYPE:	// ��ü Ÿ�� �޺��ڽ� ���� ������ ������ �Է� �ʵ尡 �޶��� (�����ؾ� �ϹǷ� Cell ���� �ҷ����� ����)
					//////////////////////////////////////////////////////////// �ʵ� ���� (Ŭ���� ��)
					// �ϴ� �׸��� �����, ��ü Ÿ�� ���� �׸� ǥ����
					DGHideItem (dialogID, LABEL_WIDTH);
					DGHideItem (dialogID, EDITCONTROL_WIDTH);
					DGHideItem (dialogID, LABEL_HEIGHT);
					DGHideItem (dialogID, EDITCONTROL_HEIGHT);
					DGHideItem (dialogID, LABEL_THK);
					DGHideItem (dialogID, EDITCONTROL_THK);
					DGHideItem (dialogID, LABEL_ORIENTATION);
					DGHideItem (dialogID, RADIO_ORIENTATION_1_PLYWOOD);
					DGHideItem (dialogID, RADIO_ORIENTATION_2_PLYWOOD);
					DGHideItem (dialogID, CHECKBOX_SET_STANDARD);
					DGHideItem (dialogID, LABEL_EUROFORM_WIDTH_OPTIONS);
					DGHideItem (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS);
					DGHideItem (dialogID, EDITCONTROL_EUROFORM_WIDTH_OPTIONS);
					DGHideItem (dialogID, LABEL_EUROFORM_HEIGHT_OPTIONS);
					DGHideItem (dialogID, POPUP_EUROFORM_HEIGHT_OPTIONS);
					DGHideItem (dialogID, EDITCONTROL_EUROFORM_HEIGHT_OPTIONS);
					DGHideItem (dialogID, LABEL_EUROFORM_ORIENTATION_OPTIONS);
					DGHideItem (dialogID, RADIO_ORIENTATION_1_EUROFORM);
					DGHideItem (dialogID, RADIO_ORIENTATION_2_EUROFORM);

					DGShowItem (dialogID, LABEL_OBJ_TYPE);
					DGShowItem (dialogID, POPUP_OBJ_TYPE);

					if (DGPopUpGetSelected (dialogID, POPUP_OBJ_TYPE) == INCORNER + 1) {
						// ��: �ʺ�
						DGShowItem (dialogID, LABEL_WIDTH);

						// Edit ��Ʈ��: �ʺ�
						DGShowItem (dialogID, EDITCONTROL_WIDTH);
						DGSetItemMinDouble (dialogID, EDITCONTROL_WIDTH, 0.080);
						DGSetItemMaxDouble (dialogID, EDITCONTROL_WIDTH, 0.500);

						// �� ����
						DGShowItem (dialogID, LABEL_HEIGHT);

						// Edit ��Ʈ��: ����
						DGShowItem (dialogID, EDITCONTROL_HEIGHT);
						DGSetItemMinDouble (dialogID, EDITCONTROL_HEIGHT, 0.050);
						DGSetItemMaxDouble (dialogID, EDITCONTROL_HEIGHT, 1.500);

					} else if (DGPopUpGetSelected (dialogID, POPUP_OBJ_TYPE) == EUROFORM + 1) {
						// üũ�ڽ�: �԰���
						DGShowItem (dialogID, CHECKBOX_SET_STANDARD);
						DGSetItemValLong (dialogID, CHECKBOX_SET_STANDARD, true);

						// ��: �ʺ�
						DGShowItem (dialogID, LABEL_EUROFORM_WIDTH_OPTIONS);

						// ��: ����
						DGShowItem (dialogID, LABEL_EUROFORM_HEIGHT_OPTIONS);

						// �԰����̸�,
						if (DGGetItemValLong (dialogID, CHECKBOX_SET_STANDARD) == TRUE) {
							// �˾� ��Ʈ��: �ʺ�
							DGShowItem (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS);
							DGHideItem (dialogID, EDITCONTROL_EUROFORM_WIDTH_OPTIONS);

							// �˾� ��Ʈ��: ����
							DGShowItem (dialogID, POPUP_EUROFORM_HEIGHT_OPTIONS);
							DGHideItem (dialogID, EDITCONTROL_EUROFORM_HEIGHT_OPTIONS);
						} else if (DGGetItemValLong (dialogID, CHECKBOX_SET_STANDARD) == FALSE) {
							// Edit ��Ʈ��: �ʺ�
							DGHideItem (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS);
							DGShowItem (dialogID, EDITCONTROL_EUROFORM_WIDTH_OPTIONS);
							DGSetItemMinDouble (dialogID, EDITCONTROL_EUROFORM_WIDTH_OPTIONS, 0.050);
							DGSetItemMaxDouble (dialogID, EDITCONTROL_EUROFORM_WIDTH_OPTIONS, 0.900);

							// Edit ��Ʈ��: ����
							DGHideItem (dialogID, POPUP_EUROFORM_HEIGHT_OPTIONS);
							DGShowItem (dialogID, EDITCONTROL_EUROFORM_HEIGHT_OPTIONS);
							DGSetItemMinDouble (dialogID, EDITCONTROL_EUROFORM_HEIGHT_OPTIONS, 0.050);
							DGSetItemMaxDouble (dialogID, EDITCONTROL_EUROFORM_HEIGHT_OPTIONS, 1.500);
						}

						// ��: ��ġ����
						DGShowItem (dialogID, LABEL_EUROFORM_ORIENTATION_OPTIONS);
				
						// ���� ��ư: ��ġ���� (�������)
						DGShowItem (dialogID, RADIO_ORIENTATION_1_EUROFORM);
						// ���� ��ư: ��ġ���� (��������)
						DGShowItem (dialogID, RADIO_ORIENTATION_2_EUROFORM);

						DGSetItemValLong (dialogID, RADIO_ORIENTATION_1_EUROFORM, true);
						DGSetItemValLong (dialogID, RADIO_ORIENTATION_2_EUROFORM, false);

					} else if (DGPopUpGetSelected (dialogID, POPUP_OBJ_TYPE) == FILLERSPACER + 1) {
						// ��: �ʺ�
						DGShowItem (dialogID, LABEL_WIDTH);

						// Edit ��Ʈ��: �ʺ�
						DGShowItem (dialogID, EDITCONTROL_WIDTH);
						DGSetItemMinDouble (dialogID, EDITCONTROL_WIDTH, 0.010);
						DGSetItemMaxDouble (dialogID, EDITCONTROL_WIDTH, 0.050);

						// �� ����
						DGShowItem (dialogID, LABEL_HEIGHT);

						// Edit ��Ʈ��: ����
						DGShowItem (dialogID, EDITCONTROL_HEIGHT);
						DGSetItemMinDouble (dialogID, EDITCONTROL_HEIGHT, 0.150);
						DGSetItemMaxDouble (dialogID, EDITCONTROL_HEIGHT, 2.400);

					} else if (DGPopUpGetSelected (dialogID, POPUP_OBJ_TYPE) == PLYWOOD + 1) {
						// ��: �ʺ�
						DGShowItem (dialogID, LABEL_WIDTH);

						// Edit ��Ʈ��: �ʺ�
						DGShowItem (dialogID, EDITCONTROL_WIDTH);
						DGSetItemMinDouble (dialogID, EDITCONTROL_WIDTH, 0.110);
						DGSetItemMaxDouble (dialogID, EDITCONTROL_WIDTH, 1.220);

						// ��: ����
						DGShowItem (dialogID, LABEL_HEIGHT);

						// Edit ��Ʈ��: ����
						DGShowItem (dialogID, EDITCONTROL_HEIGHT);
						DGSetItemMinDouble (dialogID, EDITCONTROL_HEIGHT, 0.110);
						DGSetItemMaxDouble (dialogID, EDITCONTROL_HEIGHT, 2.440);

						// ��: ��ġ����
						DGShowItem (dialogID, LABEL_ORIENTATION);
				
						// ���� ��ư: ��ġ���� (�������)
						DGShowItem (dialogID, RADIO_ORIENTATION_1_PLYWOOD);
						// ���� ��ư: ��ġ���� (��������)
						DGShowItem (dialogID, RADIO_ORIENTATION_2_PLYWOOD);

						DGSetItemValLong (dialogID, RADIO_ORIENTATION_1_PLYWOOD, true);
						DGSetItemValLong (dialogID, RADIO_ORIENTATION_2_PLYWOOD, false);
					} else if (DGPopUpGetSelected (dialogID, POPUP_OBJ_TYPE) == WOOD + 1) {
						// ��: �ʺ�
						DGShowItem (dialogID, LABEL_WIDTH);

						// Edit ��Ʈ��: �ʺ�
						DGShowItem (dialogID, EDITCONTROL_WIDTH);
						DGSetItemMinDouble (dialogID, EDITCONTROL_WIDTH, 0.005);
						DGSetItemMaxDouble (dialogID, EDITCONTROL_WIDTH, 1.000);

						// ��: ����
						DGShowItem (dialogID, LABEL_HEIGHT);

						// Edit ��Ʈ��: ����
						DGShowItem (dialogID, EDITCONTROL_HEIGHT);
						DGSetItemMinDouble (dialogID, EDITCONTROL_HEIGHT, 0.010);
						DGSetItemMaxDouble (dialogID, EDITCONTROL_HEIGHT, 3.600);

						// ��: �β�
						DGShowItem (dialogID, LABEL_THK);

						// Edit ��Ʈ��: �β�
						DGShowItem (dialogID, EDITCONTROL_THK);
						DGSetItemMinDouble (dialogID, EDITCONTROL_THK, 0.005);
						DGSetItemMaxDouble (dialogID, EDITCONTROL_THK, 1.000);
					}

					break;

				case POPUP_OBJ_TYPE_PREV:	// ��ü Ÿ�� �޺��ڽ� ���� ������ ������ �Է� �ʵ尡 �޶��� (�����ؾ� �ϹǷ� Cell ���� �ҷ����� ����)
					//////////////////////////////////////////////////////////// �ʵ� ���� (���� ��)
					// �ϴ� �׸��� �����, ��ü Ÿ�� ���� �׸� ǥ����
					DGHideItem (dialogID, LABEL_WIDTH_PREV);
					DGHideItem (dialogID, EDITCONTROL_WIDTH_PREV);
					DGHideItem (dialogID, LABEL_HEIGHT_PREV);
					DGHideItem (dialogID, EDITCONTROL_HEIGHT_PREV);
					DGHideItem (dialogID, LABEL_THK_PREV);
					DGHideItem (dialogID, EDITCONTROL_THK_PREV);
					DGHideItem (dialogID, LABEL_ORIENTATION_PREV);
					DGHideItem (dialogID, RADIO_ORIENTATION_1_PLYWOOD_PREV);
					DGHideItem (dialogID, RADIO_ORIENTATION_2_PLYWOOD_PREV);
					DGHideItem (dialogID, CHECKBOX_SET_STANDARD_PREV);
					DGHideItem (dialogID, LABEL_EUROFORM_WIDTH_OPTIONS_PREV);
					DGHideItem (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS_PREV);
					DGHideItem (dialogID, EDITCONTROL_EUROFORM_WIDTH_OPTIONS_PREV);
					DGHideItem (dialogID, LABEL_EUROFORM_HEIGHT_OPTIONS_PREV);
					DGHideItem (dialogID, POPUP_EUROFORM_HEIGHT_OPTIONS_PREV);
					DGHideItem (dialogID, EDITCONTROL_EUROFORM_HEIGHT_OPTIONS_PREV);
					DGHideItem (dialogID, LABEL_EUROFORM_ORIENTATION_OPTIONS_PREV);
					DGHideItem (dialogID, RADIO_ORIENTATION_1_EUROFORM_PREV);
					DGHideItem (dialogID, RADIO_ORIENTATION_2_EUROFORM_PREV);

					DGShowItem (dialogID, LABEL_OBJ_TYPE_PREV);
					DGShowItem (dialogID, POPUP_OBJ_TYPE_PREV);

					if (DGPopUpGetSelected (dialogID, POPUP_OBJ_TYPE_PREV) == INCORNER + 1) {
						// ��: �ʺ�
						DGShowItem (dialogID, LABEL_WIDTH_PREV);

						// Edit ��Ʈ��: �ʺ�
						DGShowItem (dialogID, EDITCONTROL_WIDTH_PREV);
						DGSetItemMinDouble (dialogID, EDITCONTROL_WIDTH_PREV, 0.080);
						DGSetItemMaxDouble (dialogID, EDITCONTROL_WIDTH_PREV, 0.500);

						// �� ����
						DGShowItem (dialogID, LABEL_HEIGHT_PREV);

						// Edit ��Ʈ��: ����
						DGShowItem (dialogID, EDITCONTROL_HEIGHT_PREV);
						DGSetItemMinDouble (dialogID, EDITCONTROL_HEIGHT_PREV, 0.050);
						DGSetItemMaxDouble (dialogID, EDITCONTROL_HEIGHT_PREV, 1.500);

					} else if (DGPopUpGetSelected (dialogID, POPUP_OBJ_TYPE_PREV) == EUROFORM + 1) {
						// üũ�ڽ�: �԰���
						DGShowItem (dialogID, CHECKBOX_SET_STANDARD_PREV);
						DGSetItemValLong (dialogID, CHECKBOX_SET_STANDARD_PREV, true);

						// ��: �ʺ�
						DGShowItem (dialogID, LABEL_EUROFORM_WIDTH_OPTIONS_PREV);

						// ��: ����
						DGShowItem (dialogID, LABEL_EUROFORM_HEIGHT_OPTIONS_PREV);

						// �԰����̸�,
						if (DGGetItemValLong (dialogID, CHECKBOX_SET_STANDARD_PREV) == TRUE) {
							// �˾� ��Ʈ��: �ʺ�
							DGShowItem (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS_PREV);
							DGHideItem (dialogID, EDITCONTROL_EUROFORM_WIDTH_OPTIONS_PREV);

							// �˾� ��Ʈ��: ����
							DGShowItem (dialogID, POPUP_EUROFORM_HEIGHT_OPTIONS_PREV);
							DGHideItem (dialogID, EDITCONTROL_EUROFORM_HEIGHT_OPTIONS_PREV);
						} else if (DGGetItemValLong (dialogID, CHECKBOX_SET_STANDARD_PREV) == FALSE) {
							// Edit ��Ʈ��: �ʺ�
							DGHideItem (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS_PREV);
							DGShowItem (dialogID, EDITCONTROL_EUROFORM_WIDTH_OPTIONS_PREV);
							DGSetItemMinDouble (dialogID, EDITCONTROL_EUROFORM_WIDTH_OPTIONS_PREV, 0.050);
							DGSetItemMaxDouble (dialogID, EDITCONTROL_EUROFORM_WIDTH_OPTIONS_PREV, 0.900);

							// Edit ��Ʈ��: ����
							DGHideItem (dialogID, POPUP_EUROFORM_HEIGHT_OPTIONS_PREV);
							DGShowItem (dialogID, EDITCONTROL_EUROFORM_HEIGHT_OPTIONS_PREV);
							DGSetItemMinDouble (dialogID, EDITCONTROL_EUROFORM_HEIGHT_OPTIONS_PREV, 0.050);
							DGSetItemMaxDouble (dialogID, EDITCONTROL_EUROFORM_HEIGHT_OPTIONS_PREV, 1.500);
						}

						// ��: ��ġ����
						DGShowItem (dialogID, LABEL_EUROFORM_ORIENTATION_OPTIONS_PREV);
				
						// ���� ��ư: ��ġ���� (�������)
						DGShowItem (dialogID, RADIO_ORIENTATION_1_EUROFORM_PREV);
						// ���� ��ư: ��ġ���� (��������)
						DGShowItem (dialogID, RADIO_ORIENTATION_2_EUROFORM_PREV);

						DGSetItemValLong (dialogID, RADIO_ORIENTATION_1_EUROFORM_PREV, true);
						DGSetItemValLong (dialogID, RADIO_ORIENTATION_2_EUROFORM_PREV, false);

					} else if (DGPopUpGetSelected (dialogID, POPUP_OBJ_TYPE_PREV) == FILLERSPACER + 1) {
						// ��: �ʺ�
						DGShowItem (dialogID, LABEL_WIDTH_PREV);

						// Edit ��Ʈ��: �ʺ�
						DGShowItem (dialogID, EDITCONTROL_WIDTH_PREV);
						DGSetItemMinDouble (dialogID, EDITCONTROL_WIDTH_PREV, 0.010);
						DGSetItemMaxDouble (dialogID, EDITCONTROL_WIDTH_PREV, 0.050);

						// �� ����
						DGShowItem (dialogID, LABEL_HEIGHT_PREV);

						// Edit ��Ʈ��: ����
						DGShowItem (dialogID, EDITCONTROL_HEIGHT_PREV);
						DGSetItemMinDouble (dialogID, EDITCONTROL_HEIGHT_PREV, 0.150);
						DGSetItemMaxDouble (dialogID, EDITCONTROL_HEIGHT_PREV, 2.400);

					} else if (DGPopUpGetSelected (dialogID, POPUP_OBJ_TYPE_PREV) == PLYWOOD + 1) {
						// ��: �ʺ�
						DGShowItem (dialogID, LABEL_WIDTH_PREV);

						// Edit ��Ʈ��: �ʺ�
						DGShowItem (dialogID, EDITCONTROL_WIDTH_PREV);
						DGSetItemMinDouble (dialogID, EDITCONTROL_WIDTH_PREV, 0.110);
						DGSetItemMaxDouble (dialogID, EDITCONTROL_WIDTH_PREV, 1.220);

						// ��: ����
						DGShowItem (dialogID, LABEL_HEIGHT_PREV);

						// Edit ��Ʈ��: ����
						DGShowItem (dialogID, EDITCONTROL_HEIGHT_PREV);
						DGSetItemMinDouble (dialogID, EDITCONTROL_HEIGHT_PREV, 0.110);
						DGSetItemMaxDouble (dialogID, EDITCONTROL_HEIGHT_PREV, 2.440);

						// ��: ��ġ����
						DGShowItem (dialogID, LABEL_ORIENTATION_PREV);
				
						// ���� ��ư: ��ġ���� (�������)
						DGShowItem (dialogID, RADIO_ORIENTATION_1_PLYWOOD_PREV);
						// ���� ��ư: ��ġ���� (��������)
						DGShowItem (dialogID, RADIO_ORIENTATION_2_PLYWOOD_PREV);

						DGSetItemValLong (dialogID, RADIO_ORIENTATION_1_PLYWOOD_PREV, true);
						DGSetItemValLong (dialogID, RADIO_ORIENTATION_2_PLYWOOD_PREV, false);
					} else if (DGPopUpGetSelected (dialogID, POPUP_OBJ_TYPE_PREV) == WOOD + 1) {
						// ��: �ʺ�
						DGShowItem (dialogID, LABEL_WIDTH_PREV);

						// Edit ��Ʈ��: �ʺ�
						DGShowItem (dialogID, EDITCONTROL_WIDTH_PREV);
						DGSetItemMinDouble (dialogID, EDITCONTROL_WIDTH_PREV, 0.005);
						DGSetItemMaxDouble (dialogID, EDITCONTROL_WIDTH_PREV, 1.000);

						// ��: ����
						DGShowItem (dialogID, LABEL_HEIGHT_PREV);

						// Edit ��Ʈ��: ����
						DGShowItem (dialogID, EDITCONTROL_HEIGHT_PREV);
						DGSetItemMinDouble (dialogID, EDITCONTROL_HEIGHT_PREV, 0.010);
						DGSetItemMaxDouble (dialogID, EDITCONTROL_HEIGHT_PREV, 3.600);

						// ��: �β�
						DGShowItem (dialogID, LABEL_THK_PREV);

						// Edit ��Ʈ��: �β�
						DGShowItem (dialogID, EDITCONTROL_THK_PREV);
						DGSetItemMinDouble (dialogID, EDITCONTROL_THK_PREV, 0.005);
						DGSetItemMaxDouble (dialogID, EDITCONTROL_THK_PREV, 1.000);
					}

					break;

				case POPUP_OBJ_TYPE_NEXT:	// ��ü Ÿ�� �޺��ڽ� ���� ������ ������ �Է� �ʵ尡 �޶��� (�����ؾ� �ϹǷ� Cell ���� �ҷ����� ����)
					//////////////////////////////////////////////////////////// �ʵ� ���� (���� ��)
					// �ϴ� �׸��� �����, ��ü Ÿ�� ���� �׸� ǥ����
					DGHideItem (dialogID, LABEL_WIDTH_NEXT);
					DGHideItem (dialogID, EDITCONTROL_WIDTH_NEXT);
					DGHideItem (dialogID, LABEL_HEIGHT_NEXT);
					DGHideItem (dialogID, EDITCONTROL_HEIGHT_NEXT);
					DGHideItem (dialogID, LABEL_THK_NEXT);
					DGHideItem (dialogID, EDITCONTROL_THK_NEXT);
					DGHideItem (dialogID, LABEL_ORIENTATION_NEXT);
					DGHideItem (dialogID, RADIO_ORIENTATION_1_PLYWOOD_NEXT);
					DGHideItem (dialogID, RADIO_ORIENTATION_2_PLYWOOD_NEXT);
					DGHideItem (dialogID, CHECKBOX_SET_STANDARD_NEXT);
					DGHideItem (dialogID, LABEL_EUROFORM_WIDTH_OPTIONS_NEXT);
					DGHideItem (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS_NEXT);
					DGHideItem (dialogID, EDITCONTROL_EUROFORM_WIDTH_OPTIONS_NEXT);
					DGHideItem (dialogID, LABEL_EUROFORM_HEIGHT_OPTIONS_NEXT);
					DGHideItem (dialogID, POPUP_EUROFORM_HEIGHT_OPTIONS_NEXT);
					DGHideItem (dialogID, EDITCONTROL_EUROFORM_HEIGHT_OPTIONS_NEXT);
					DGHideItem (dialogID, LABEL_EUROFORM_ORIENTATION_OPTIONS_NEXT);
					DGHideItem (dialogID, RADIO_ORIENTATION_1_EUROFORM_NEXT);
					DGHideItem (dialogID, RADIO_ORIENTATION_2_EUROFORM_NEXT);

					DGShowItem (dialogID, LABEL_OBJ_TYPE_NEXT);
					DGShowItem (dialogID, POPUP_OBJ_TYPE_NEXT);

					if (DGPopUpGetSelected (dialogID, POPUP_OBJ_TYPE_NEXT) == INCORNER + 1) {
						// ��: �ʺ�
						DGShowItem (dialogID, LABEL_WIDTH_NEXT);

						// Edit ��Ʈ��: �ʺ�
						DGShowItem (dialogID, EDITCONTROL_WIDTH_NEXT);
						DGSetItemMinDouble (dialogID, EDITCONTROL_WIDTH_NEXT, 0.080);
						DGSetItemMaxDouble (dialogID, EDITCONTROL_WIDTH_NEXT, 0.500);

						// �� ����
						DGShowItem (dialogID, LABEL_HEIGHT_NEXT);

						// Edit ��Ʈ��: ����
						DGShowItem (dialogID, EDITCONTROL_HEIGHT_NEXT);
						DGSetItemMinDouble (dialogID, EDITCONTROL_HEIGHT_NEXT, 0.050);
						DGSetItemMaxDouble (dialogID, EDITCONTROL_HEIGHT_NEXT, 1.500);

					} else if (DGPopUpGetSelected (dialogID, POPUP_OBJ_TYPE_NEXT) == EUROFORM + 1) {
						// üũ�ڽ�: �԰���
						DGShowItem (dialogID, CHECKBOX_SET_STANDARD_NEXT);
						DGSetItemValLong (dialogID, CHECKBOX_SET_STANDARD_NEXT, true);

						// ��: �ʺ�
						DGShowItem (dialogID, LABEL_EUROFORM_WIDTH_OPTIONS_NEXT);

						// ��: ����
						DGShowItem (dialogID, LABEL_EUROFORM_HEIGHT_OPTIONS_NEXT);

						// �԰����̸�,
						if (DGGetItemValLong (dialogID, CHECKBOX_SET_STANDARD_NEXT) == TRUE) {
							// �˾� ��Ʈ��: �ʺ�
							DGShowItem (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS_NEXT);
							DGHideItem (dialogID, EDITCONTROL_EUROFORM_WIDTH_OPTIONS_NEXT);

							// �˾� ��Ʈ��: ����
							DGShowItem (dialogID, POPUP_EUROFORM_HEIGHT_OPTIONS_NEXT);
							DGHideItem (dialogID, EDITCONTROL_EUROFORM_HEIGHT_OPTIONS_NEXT);
						} else if (DGGetItemValLong (dialogID, CHECKBOX_SET_STANDARD_NEXT) == FALSE) {
							// Edit ��Ʈ��: �ʺ�
							DGHideItem (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS_NEXT);
							DGShowItem (dialogID, EDITCONTROL_EUROFORM_WIDTH_OPTIONS_NEXT);
							DGSetItemMinDouble (dialogID, EDITCONTROL_EUROFORM_WIDTH_OPTIONS_NEXT, 0.050);
							DGSetItemMaxDouble (dialogID, EDITCONTROL_EUROFORM_WIDTH_OPTIONS_NEXT, 0.900);

							// Edit ��Ʈ��: ����
							DGHideItem (dialogID, POPUP_EUROFORM_HEIGHT_OPTIONS_NEXT);
							DGShowItem (dialogID, EDITCONTROL_EUROFORM_HEIGHT_OPTIONS_NEXT);
							DGSetItemMinDouble (dialogID, EDITCONTROL_EUROFORM_HEIGHT_OPTIONS_NEXT, 0.050);
							DGSetItemMaxDouble (dialogID, EDITCONTROL_EUROFORM_HEIGHT_OPTIONS_NEXT, 1.500);
						}

						// ��: ��ġ����
						DGShowItem (dialogID, LABEL_EUROFORM_ORIENTATION_OPTIONS_NEXT);
				
						// ���� ��ư: ��ġ���� (�������)
						DGShowItem (dialogID, RADIO_ORIENTATION_1_EUROFORM_NEXT);
						// ���� ��ư: ��ġ���� (��������)
						DGShowItem (dialogID, RADIO_ORIENTATION_2_EUROFORM_NEXT);

						DGSetItemValLong (dialogID, RADIO_ORIENTATION_1_EUROFORM_NEXT, true);
						DGSetItemValLong (dialogID, RADIO_ORIENTATION_2_EUROFORM_NEXT, false);

					} else if (DGPopUpGetSelected (dialogID, POPUP_OBJ_TYPE_NEXT) == FILLERSPACER + 1) {
						// ��: �ʺ�
						DGShowItem (dialogID, LABEL_WIDTH_NEXT);

						// Edit ��Ʈ��: �ʺ�
						DGShowItem (dialogID, EDITCONTROL_WIDTH_NEXT);
						DGSetItemMinDouble (dialogID, EDITCONTROL_WIDTH_NEXT, 0.010);
						DGSetItemMaxDouble (dialogID, EDITCONTROL_WIDTH_NEXT, 0.050);

						// �� ����
						DGShowItem (dialogID, LABEL_HEIGHT_NEXT);

						// Edit ��Ʈ��: ����
						DGShowItem (dialogID, EDITCONTROL_HEIGHT_NEXT);
						DGSetItemMinDouble (dialogID, EDITCONTROL_HEIGHT_NEXT, 0.150);
						DGSetItemMaxDouble (dialogID, EDITCONTROL_HEIGHT_NEXT, 2.400);

					} else if (DGPopUpGetSelected (dialogID, POPUP_OBJ_TYPE_NEXT) == PLYWOOD + 1) {
						// ��: �ʺ�
						DGShowItem (dialogID, LABEL_WIDTH_NEXT);

						// Edit ��Ʈ��: �ʺ�
						DGShowItem (dialogID, EDITCONTROL_WIDTH_NEXT);
						DGSetItemMinDouble (dialogID, EDITCONTROL_WIDTH_NEXT, 0.110);
						DGSetItemMaxDouble (dialogID, EDITCONTROL_WIDTH_NEXT, 1.220);

						// ��: ����
						DGShowItem (dialogID, LABEL_HEIGHT_NEXT);

						// Edit ��Ʈ��: ����
						DGShowItem (dialogID, EDITCONTROL_HEIGHT_NEXT);
						DGSetItemMinDouble (dialogID, EDITCONTROL_HEIGHT_NEXT, 0.110);
						DGSetItemMaxDouble (dialogID, EDITCONTROL_HEIGHT_NEXT, 2.440);

						// ��: ��ġ����
						DGShowItem (dialogID, LABEL_ORIENTATION_NEXT);
				
						// ���� ��ư: ��ġ���� (�������)
						DGShowItem (dialogID, RADIO_ORIENTATION_1_PLYWOOD_NEXT);
						// ���� ��ư: ��ġ���� (��������)
						DGShowItem (dialogID, RADIO_ORIENTATION_2_PLYWOOD_NEXT);

						DGSetItemValLong (dialogID, RADIO_ORIENTATION_1_PLYWOOD_NEXT, true);
						DGSetItemValLong (dialogID, RADIO_ORIENTATION_2_PLYWOOD_NEXT, false);
					} else if (DGPopUpGetSelected (dialogID, POPUP_OBJ_TYPE_NEXT) == WOOD + 1) {
						// ��: �ʺ�
						DGShowItem (dialogID, LABEL_WIDTH_NEXT);

						// Edit ��Ʈ��: �ʺ�
						DGShowItem (dialogID, EDITCONTROL_WIDTH_NEXT);
						DGSetItemMinDouble (dialogID, EDITCONTROL_WIDTH_NEXT, 0.005);
						DGSetItemMaxDouble (dialogID, EDITCONTROL_WIDTH_NEXT, 1.000);

						// ��: ����
						DGShowItem (dialogID, LABEL_HEIGHT_NEXT);

						// Edit ��Ʈ��: ����
						DGShowItem (dialogID, EDITCONTROL_HEIGHT_NEXT);
						DGSetItemMinDouble (dialogID, EDITCONTROL_HEIGHT_NEXT, 0.010);
						DGSetItemMaxDouble (dialogID, EDITCONTROL_HEIGHT_NEXT, 3.600);

						// ��: �β�
						DGShowItem (dialogID, LABEL_THK_NEXT);

						// Edit ��Ʈ��: �β�
						DGShowItem (dialogID, EDITCONTROL_THK_NEXT);
						DGSetItemMinDouble (dialogID, EDITCONTROL_THK_NEXT, 0.005);
						DGSetItemMaxDouble (dialogID, EDITCONTROL_THK_NEXT, 1.000);
					}

					break;

				case CHECKBOX_SET_STANDARD:	// �������� ���, �԰��� üũ�ڽ� ���� �ٲ� ������ �ʺ�, ���� �Է� �ʵ� Ÿ���� �ٲ�
					//////////////////////////////////////////////////////////// �ʵ� ���� (Ŭ���� ��)
					if (DGGetItemValLong (dialogID, CHECKBOX_SET_STANDARD) == TRUE) {
						// �˾� ��Ʈ��: �ʺ�
						DGShowItem (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS);
						DGHideItem (dialogID, EDITCONTROL_EUROFORM_WIDTH_OPTIONS);
						// �˾� ��Ʈ��: ����
						DGShowItem (dialogID, POPUP_EUROFORM_HEIGHT_OPTIONS);
						DGHideItem (dialogID, EDITCONTROL_EUROFORM_HEIGHT_OPTIONS);
					} else if (DGGetItemValLong (dialogID, CHECKBOX_SET_STANDARD) == FALSE) {
						// Edit ��Ʈ��: �ʺ�
						DGHideItem (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS);
						DGShowItem (dialogID, EDITCONTROL_EUROFORM_WIDTH_OPTIONS);
						DGSetItemMinDouble (dialogID, EDITCONTROL_EUROFORM_WIDTH_OPTIONS, 0.050);
						DGSetItemMaxDouble (dialogID, EDITCONTROL_EUROFORM_WIDTH_OPTIONS, 0.900);
						// Edit ��Ʈ��: ����
						DGHideItem (dialogID, POPUP_EUROFORM_HEIGHT_OPTIONS);
						DGShowItem (dialogID, EDITCONTROL_EUROFORM_HEIGHT_OPTIONS);
						DGSetItemMinDouble (dialogID, EDITCONTROL_EUROFORM_HEIGHT_OPTIONS, 0.050);
						DGSetItemMaxDouble (dialogID, EDITCONTROL_EUROFORM_HEIGHT_OPTIONS, 1.500);
					}

					break;

				case CHECKBOX_SET_STANDARD_PREV:	// �������� ���, �԰��� üũ�ڽ� ���� �ٲ� ������ �ʺ�, ���� �Է� �ʵ� Ÿ���� �ٲ�
					//////////////////////////////////////////////////////////// �ʵ� ���� (���� ��)
					if (DGGetItemValLong (dialogID, CHECKBOX_SET_STANDARD_PREV) == TRUE) {
						// �˾� ��Ʈ��: �ʺ�
						DGShowItem (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS_PREV);
						DGHideItem (dialogID, EDITCONTROL_EUROFORM_WIDTH_OPTIONS_PREV);
						// �˾� ��Ʈ��: ����
						DGShowItem (dialogID, POPUP_EUROFORM_HEIGHT_OPTIONS_PREV);
						DGHideItem (dialogID, EDITCONTROL_EUROFORM_HEIGHT_OPTIONS_PREV);
					} else if (DGGetItemValLong (dialogID, CHECKBOX_SET_STANDARD_PREV) == FALSE) {
						// Edit ��Ʈ��: �ʺ�
						DGHideItem (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS_PREV);
						DGShowItem (dialogID, EDITCONTROL_EUROFORM_WIDTH_OPTIONS_PREV);
						DGSetItemMinDouble (dialogID, EDITCONTROL_EUROFORM_WIDTH_OPTIONS_PREV, 0.050);
						DGSetItemMaxDouble (dialogID, EDITCONTROL_EUROFORM_WIDTH_OPTIONS_PREV, 0.900);
						// Edit ��Ʈ��: ����
						DGHideItem (dialogID, POPUP_EUROFORM_HEIGHT_OPTIONS_PREV);
						DGShowItem (dialogID, EDITCONTROL_EUROFORM_HEIGHT_OPTIONS_PREV);
						DGSetItemMinDouble (dialogID, EDITCONTROL_EUROFORM_HEIGHT_OPTIONS_PREV, 0.050);
						DGSetItemMaxDouble (dialogID, EDITCONTROL_EUROFORM_HEIGHT_OPTIONS_PREV, 1.500);
					}

					break;

				case CHECKBOX_SET_STANDARD_NEXT:	// �������� ���, �԰��� üũ�ڽ� ���� �ٲ� ������ �ʺ�, ���� �Է� �ʵ� Ÿ���� �ٲ�
					//////////////////////////////////////////////////////////// �ʵ� ���� (���� ��)
					if (DGGetItemValLong (dialogID, CHECKBOX_SET_STANDARD_NEXT) == TRUE) {
						// �˾� ��Ʈ��: �ʺ�
						DGShowItem (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS_NEXT);
						DGHideItem (dialogID, EDITCONTROL_EUROFORM_WIDTH_OPTIONS_NEXT);
						// �˾� ��Ʈ��: ����
						DGShowItem (dialogID, POPUP_EUROFORM_HEIGHT_OPTIONS_NEXT);
						DGHideItem (dialogID, EDITCONTROL_EUROFORM_HEIGHT_OPTIONS_NEXT);
					} else if (DGGetItemValLong (dialogID, CHECKBOX_SET_STANDARD_NEXT) == FALSE) {
						// Edit ��Ʈ��: �ʺ�
						DGHideItem (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS_NEXT);
						DGShowItem (dialogID, EDITCONTROL_EUROFORM_WIDTH_OPTIONS_NEXT);
						DGSetItemMinDouble (dialogID, EDITCONTROL_EUROFORM_WIDTH_OPTIONS_NEXT, 0.050);
						DGSetItemMaxDouble (dialogID, EDITCONTROL_EUROFORM_WIDTH_OPTIONS_NEXT, 0.900);
						// Edit ��Ʈ��: ����
						DGHideItem (dialogID, POPUP_EUROFORM_HEIGHT_OPTIONS_NEXT);
						DGShowItem (dialogID, EDITCONTROL_EUROFORM_HEIGHT_OPTIONS_NEXT);
						DGSetItemMinDouble (dialogID, EDITCONTROL_EUROFORM_HEIGHT_OPTIONS_NEXT, 0.050);
						DGSetItemMaxDouble (dialogID, EDITCONTROL_EUROFORM_HEIGHT_OPTIONS_NEXT, 1.500);
					}

					break;
			}

		case DG_MSG_CLICK:
			switch (item) {
				case DG_OK:

					// wallPlacerHandlerSecondary ���� Ŭ���� �׸��� ��ư�� �ε��� ���� �̿��Ͽ� �� �ε��� �� �ε�
					idxCell = (clickedBtnItemIdx - itemInitIdx) * 2;
					while (idxCell >= ((placingZone.eu_count_hor + 2) * 2))
						idxCell -= ((placingZone.eu_count_hor + 2) * 2);

					// ���� ���� �߰� ���̸�,
					if ( (idxCell > 0) && (idxCell < (placingZone.nCells - 1)) ) {
						idxCell_prev = idxCell - 1;
						idxCell_next = idxCell + 1;
					// ���� ���� �� ó�� ���̸�,
					} else if (idxCell == 0) {
						idxCell_prev = -1;
						idxCell_next = idxCell + 1;
					// ���� ���� �� �� ���̸�,
					} else if (idxCell == (placingZone.nCells - 1)) {
						idxCell_prev = idxCell - 1;
						idxCell_next = -1;
					}

					for (xx = 0 ; xx < placingZone.eu_count_ver ; ++xx) {

						//////////////////////////////////////////////////////////// �ʵ� ���� (Ŭ���� ��)
						// �Է��� ���� �ٽ� ���� ����
						if (DGPopUpGetSelected (dialogID, POPUP_OBJ_TYPE) == NONE + 1) {
							placingZone.cells [xx][idxCell].objType = NONE;

						} else if (DGPopUpGetSelected (dialogID, POPUP_OBJ_TYPE) == INCORNER + 1) {
							placingZone.cells [xx][idxCell].objType = INCORNER;

							// �ʺ�
							if (idxCell == 0) {
								placingZone.cells [xx][idxCell].libPart.incorner.leng_s = DGGetItemValDouble (dialogID, EDITCONTROL_WIDTH);
								placingZone.cells [xx][idxCell].libPart.incorner.wid_s = 0.100;
							} else if (idxCell > 0) {
								placingZone.cells [xx][idxCell].libPart.incorner.leng_s = 0.100;
								placingZone.cells [xx][idxCell].libPart.incorner.wid_s = DGGetItemValDouble (dialogID, EDITCONTROL_WIDTH);
							}
							placingZone.cells [xx][idxCell].horLen = DGGetItemValDouble (dialogID, EDITCONTROL_WIDTH);

							// ����
							placingZone.cells [xx][idxCell].libPart.incorner.hei_s = DGGetItemValDouble (dialogID, EDITCONTROL_HEIGHT);
							placingZone.cells [xx][idxCell].verLen = DGGetItemValDouble (dialogID, EDITCONTROL_HEIGHT);
						} else if (DGPopUpGetSelected (dialogID, POPUP_OBJ_TYPE) == EUROFORM + 1) {
							placingZone.cells [xx][idxCell].objType = EUROFORM;

							// �԰���
							if (DGGetItemValLong (dialogID, CHECKBOX_SET_STANDARD) == TRUE)
								placingZone.cells [xx][idxCell].libPart.form.eu_stan_onoff = true;
							else if (DGGetItemValLong (dialogID, CHECKBOX_SET_STANDARD) == FALSE)
								placingZone.cells [xx][idxCell].libPart.form.eu_stan_onoff = false;

							if (DGGetItemValLong (dialogID, CHECKBOX_SET_STANDARD) == TRUE) {
								// �ʺ�
								placingZone.cells [xx][idxCell].libPart.form.eu_wid = atof (DGPopUpGetItemText (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS, DGPopUpGetSelected (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS)).ToCStr ()) / 1000.0;
								placingZone.cells [xx][idxCell].horLen = placingZone.cells [xx][idxCell].libPart.form.eu_wid;
								// ����
								placingZone.cells [xx][idxCell].libPart.form.eu_hei = atof (DGPopUpGetItemText (dialogID, POPUP_EUROFORM_HEIGHT_OPTIONS, DGPopUpGetSelected (dialogID, POPUP_EUROFORM_HEIGHT_OPTIONS)).ToCStr ()) / 1000.0;
								placingZone.cells [xx][idxCell].verLen = placingZone.cells [xx][idxCell].libPart.form.eu_hei;
							} else if (DGGetItemValLong (dialogID, CHECKBOX_SET_STANDARD) == FALSE) {
								// �ʺ�
								placingZone.cells [xx][idxCell].libPart.form.eu_wid2 = DGGetItemValDouble (dialogID, EDITCONTROL_EUROFORM_WIDTH_OPTIONS);
								placingZone.cells [xx][idxCell].horLen = placingZone.cells [xx][idxCell].libPart.form.eu_wid2;
								// ����
								placingZone.cells [xx][idxCell].libPart.form.eu_hei2 = DGGetItemValDouble (dialogID, EDITCONTROL_EUROFORM_HEIGHT_OPTIONS);
								placingZone.cells [xx][idxCell].verLen = placingZone.cells [xx][idxCell].libPart.form.eu_hei2;
							}

							// ��ġ����
							if (DGGetItemValLong (dialogID, RADIO_ORIENTATION_1_EUROFORM) == TRUE)
								placingZone.cells [xx][idxCell].libPart.form.u_ins_wall = true;
							else if (DGGetItemValLong (dialogID, RADIO_ORIENTATION_1_EUROFORM) == FALSE) {
								placingZone.cells [xx][idxCell].libPart.form.u_ins_wall = false;
								// ����, ���� ���� ��ȯ
								temp = placingZone.cells [xx][idxCell].horLen;
								placingZone.cells [xx][idxCell].horLen = placingZone.cells [xx][idxCell].verLen;
								placingZone.cells [xx][idxCell].verLen = temp;
							}
						} else if (DGPopUpGetSelected (dialogID, POPUP_OBJ_TYPE) == FILLERSPACER + 1) {
							placingZone.cells [xx][idxCell].objType = FILLERSPACER;

							// �ʺ�
							placingZone.cells [xx][idxCell].libPart.fillersp.f_thk = DGGetItemValDouble (dialogID, EDITCONTROL_WIDTH);
							placingZone.cells [xx][idxCell].horLen = DGGetItemValDouble (dialogID, EDITCONTROL_WIDTH);

							// ����
							placingZone.cells [xx][idxCell].libPart.fillersp.f_leng = DGGetItemValDouble (dialogID, EDITCONTROL_HEIGHT);
							placingZone.cells [xx][idxCell].verLen = DGGetItemValDouble (dialogID, EDITCONTROL_HEIGHT);
						} else if (DGPopUpGetSelected (dialogID, POPUP_OBJ_TYPE) == PLYWOOD + 1) {
							placingZone.cells [xx][idxCell].objType = PLYWOOD;

							// �ʺ�
							placingZone.cells [xx][idxCell].libPart.plywood.p_wid = DGGetItemValDouble (dialogID, EDITCONTROL_WIDTH);
							placingZone.cells [xx][idxCell].horLen = DGGetItemValDouble (dialogID, EDITCONTROL_WIDTH);

							// ����
							placingZone.cells [xx][idxCell].libPart.plywood.p_leng = DGGetItemValDouble (dialogID, EDITCONTROL_HEIGHT);
							placingZone.cells [xx][idxCell].verLen = DGGetItemValDouble (dialogID, EDITCONTROL_HEIGHT);

							// ��ġ����
							if (DGGetItemValLong (dialogID, RADIO_ORIENTATION_1_PLYWOOD) == TRUE)
								placingZone.cells [xx][idxCell].libPart.plywood.w_dir_wall = true;
							else if (DGGetItemValLong (dialogID, RADIO_ORIENTATION_1_PLYWOOD) == FALSE) {
								placingZone.cells [xx][idxCell].libPart.plywood.w_dir_wall = false;
								// ����, ���� ���� ��ȯ
								temp = placingZone.cells [xx][idxCell].horLen;
								placingZone.cells [xx][idxCell].horLen = placingZone.cells [xx][idxCell].verLen;
								placingZone.cells [xx][idxCell].verLen = temp;
							}
						} else if (DGPopUpGetSelected (dialogID, POPUP_OBJ_TYPE) == WOOD + 1) {
							placingZone.cells [xx][idxCell].objType = WOOD;

							// �ʺ�
							placingZone.cells [xx][idxCell].libPart.wood.w_h = DGGetItemValDouble (dialogID, EDITCONTROL_WIDTH);
							placingZone.cells [xx][idxCell].horLen = DGGetItemValDouble (dialogID, EDITCONTROL_WIDTH);

							// ����
							placingZone.cells [xx][idxCell].libPart.wood.w_leng = DGGetItemValDouble (dialogID, EDITCONTROL_HEIGHT);
							placingZone.cells [xx][idxCell].verLen = DGGetItemValDouble (dialogID, EDITCONTROL_HEIGHT);

							// �β�
							placingZone.cells [xx][idxCell].libPart.wood.w_w = DGGetItemValDouble (dialogID, EDITCONTROL_THK);

							// ����: 90��
							placingZone.cells [xx][idxCell].libPart.wood.w_ang = DegreeToRad (90.0);
						}

						//////////////////////////////////////////////////////////// �ʵ� ���� (���� ��)
						if (idxCell_prev != -1) {
							// �Է��� ���� �ٽ� ���� ����
							if (DGPopUpGetSelected (dialogID, POPUP_OBJ_TYPE_PREV) == NONE + 1) {
								placingZone.cells [xx][idxCell_prev].objType = NONE;

							} else if (DGPopUpGetSelected (dialogID, POPUP_OBJ_TYPE_PREV) == INCORNER + 1) {
								placingZone.cells [xx][idxCell_prev].objType = INCORNER;

								// �ʺ�
								if (idxCell_prev == 0) {
									placingZone.cells [xx][idxCell_prev].libPart.incorner.leng_s = DGGetItemValDouble (dialogID, EDITCONTROL_WIDTH_PREV);
									placingZone.cells [xx][idxCell_prev].libPart.incorner.wid_s = 0.100;
								} else if (idxCell_prev > 0) {
									placingZone.cells [xx][idxCell_prev].libPart.incorner.leng_s = 0.100;
									placingZone.cells [xx][idxCell_prev].libPart.incorner.wid_s = DGGetItemValDouble (dialogID, EDITCONTROL_WIDTH_PREV);
								}
								placingZone.cells [xx][idxCell_prev].horLen = DGGetItemValDouble (dialogID, EDITCONTROL_WIDTH_PREV);

								// ����
								placingZone.cells [xx][idxCell_prev].libPart.incorner.hei_s = DGGetItemValDouble (dialogID, EDITCONTROL_HEIGHT_PREV);
								placingZone.cells [xx][idxCell_prev].verLen = DGGetItemValDouble (dialogID, EDITCONTROL_HEIGHT_PREV);
							} else if (DGPopUpGetSelected (dialogID, POPUP_OBJ_TYPE_PREV) == EUROFORM + 1) {
								placingZone.cells [xx][idxCell_prev].objType = EUROFORM;

								// �԰���
								if (DGGetItemValLong (dialogID, CHECKBOX_SET_STANDARD_PREV) == TRUE)
									placingZone.cells [xx][idxCell_prev].libPart.form.eu_stan_onoff = true;
								else if (DGGetItemValLong (dialogID, CHECKBOX_SET_STANDARD_PREV) == FALSE)
									placingZone.cells [xx][idxCell_prev].libPart.form.eu_stan_onoff = false;

								if (DGGetItemValLong (dialogID, CHECKBOX_SET_STANDARD_PREV) == TRUE) {
									// �ʺ�
									placingZone.cells [xx][idxCell_prev].libPart.form.eu_wid = atof (DGPopUpGetItemText (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS_PREV, DGPopUpGetSelected (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS_PREV)).ToCStr ()) / 1000.0;
									placingZone.cells [xx][idxCell_prev].horLen = placingZone.cells [xx][idxCell_prev].libPart.form.eu_wid;
									// ����
									placingZone.cells [xx][idxCell_prev].libPart.form.eu_hei = atof (DGPopUpGetItemText (dialogID, POPUP_EUROFORM_HEIGHT_OPTIONS_PREV, DGPopUpGetSelected (dialogID, POPUP_EUROFORM_HEIGHT_OPTIONS_PREV)).ToCStr ()) / 1000.0;
									placingZone.cells [xx][idxCell_prev].verLen = placingZone.cells [xx][idxCell_prev].libPart.form.eu_hei;
								} else if (DGGetItemValLong (dialogID, CHECKBOX_SET_STANDARD_PREV) == FALSE) {
									// �ʺ�
									placingZone.cells [xx][idxCell_prev].libPart.form.eu_wid2 = DGGetItemValDouble (dialogID, EDITCONTROL_EUROFORM_WIDTH_OPTIONS_PREV);
									placingZone.cells [xx][idxCell_prev].horLen = placingZone.cells [xx][idxCell_prev].libPart.form.eu_wid2;
									// ����
									placingZone.cells [xx][idxCell_prev].libPart.form.eu_hei2 = DGGetItemValDouble (dialogID, EDITCONTROL_EUROFORM_HEIGHT_OPTIONS_PREV);
									placingZone.cells [xx][idxCell_prev].verLen = placingZone.cells [xx][idxCell_prev].libPart.form.eu_hei2;
								}

								// ��ġ����
								if (DGGetItemValLong (dialogID, RADIO_ORIENTATION_1_EUROFORM_PREV) == TRUE)
									placingZone.cells [xx][idxCell_prev].libPart.form.u_ins_wall = true;
								else if (DGGetItemValLong (dialogID, RADIO_ORIENTATION_1_EUROFORM_PREV) == FALSE) {
									placingZone.cells [xx][idxCell_prev].libPart.form.u_ins_wall = false;
									// ����, ���� ���� ��ȯ
									temp = placingZone.cells [xx][idxCell_prev].horLen;
									placingZone.cells [xx][idxCell_prev].horLen = placingZone.cells [xx][idxCell_prev].verLen;
									placingZone.cells [xx][idxCell_prev].verLen = temp;
								}
							} else if (DGPopUpGetSelected (dialogID, POPUP_OBJ_TYPE_PREV) == FILLERSPACER + 1) {
								placingZone.cells [xx][idxCell_prev].objType = FILLERSPACER;

								// �ʺ�
								placingZone.cells [xx][idxCell_prev].libPart.fillersp.f_thk = DGGetItemValDouble (dialogID, EDITCONTROL_WIDTH_PREV);
								placingZone.cells [xx][idxCell_prev].horLen = DGGetItemValDouble (dialogID, EDITCONTROL_WIDTH_PREV);

								// ����
								placingZone.cells [xx][idxCell_prev].libPart.fillersp.f_leng = DGGetItemValDouble (dialogID, EDITCONTROL_HEIGHT_PREV);
								placingZone.cells [xx][idxCell_prev].verLen = DGGetItemValDouble (dialogID, EDITCONTROL_HEIGHT_PREV);
							} else if (DGPopUpGetSelected (dialogID, POPUP_OBJ_TYPE_PREV) == PLYWOOD + 1) {
								placingZone.cells [xx][idxCell_prev].objType = PLYWOOD;

								// �ʺ�
								placingZone.cells [xx][idxCell_prev].libPart.plywood.p_wid = DGGetItemValDouble (dialogID, EDITCONTROL_WIDTH_PREV);
								placingZone.cells [xx][idxCell_prev].horLen = DGGetItemValDouble (dialogID, EDITCONTROL_WIDTH_PREV);

								// ����
								placingZone.cells [xx][idxCell_prev].libPart.plywood.p_leng = DGGetItemValDouble (dialogID, EDITCONTROL_HEIGHT_PREV);
								placingZone.cells [xx][idxCell_prev].verLen = DGGetItemValDouble (dialogID, EDITCONTROL_HEIGHT_PREV);

								// ��ġ����
								if (DGGetItemValLong (dialogID, RADIO_ORIENTATION_1_PLYWOOD_PREV) == TRUE)
									placingZone.cells [xx][idxCell_prev].libPart.plywood.w_dir_wall = true;
								else if (DGGetItemValLong (dialogID, RADIO_ORIENTATION_1_PLYWOOD_PREV) == FALSE) {
									placingZone.cells [xx][idxCell_prev].libPart.plywood.w_dir_wall = false;
									// ����, ���� ���� ��ȯ
									temp = placingZone.cells [xx][idxCell_prev].horLen;
									placingZone.cells [xx][idxCell_prev].horLen = placingZone.cells [xx][idxCell_prev].verLen;
									placingZone.cells [xx][idxCell_prev].verLen = temp;
								}
							} else if (DGPopUpGetSelected (dialogID, POPUP_OBJ_TYPE_PREV) == WOOD + 1) {
								placingZone.cells [xx][idxCell_prev].objType = WOOD;

								// �ʺ�
								placingZone.cells [xx][idxCell_prev].libPart.wood.w_h = DGGetItemValDouble (dialogID, EDITCONTROL_WIDTH_PREV);
								placingZone.cells [xx][idxCell_prev].horLen = DGGetItemValDouble (dialogID, EDITCONTROL_WIDTH_PREV);

								// ����
								placingZone.cells [xx][idxCell_prev].libPart.wood.w_leng = DGGetItemValDouble (dialogID, EDITCONTROL_HEIGHT_PREV);
								placingZone.cells [xx][idxCell_prev].verLen = DGGetItemValDouble (dialogID, EDITCONTROL_HEIGHT_PREV);

								// �β�
								placingZone.cells [xx][idxCell_prev].libPart.wood.w_w = DGGetItemValDouble (dialogID, EDITCONTROL_THK_PREV);
							
								// ����: 90��
								placingZone.cells [xx][idxCell_prev].libPart.wood.w_ang = DegreeToRad (90.0);
							}
						}

						//////////////////////////////////////////////////////////// �ʵ� ���� (���� ��)
						if (idxCell_next != -1) {
							// �Է��� ���� �ٽ� ���� ����
							if (DGPopUpGetSelected (dialogID, POPUP_OBJ_TYPE_NEXT) == NONE + 1) {
								placingZone.cells [xx][idxCell_next].objType = NONE;

							} else if (DGPopUpGetSelected (dialogID, POPUP_OBJ_TYPE_NEXT) == INCORNER + 1) {
								placingZone.cells [xx][idxCell_next].objType = INCORNER;

								// �ʺ�
								if (idxCell_next == 0) {
									placingZone.cells [xx][idxCell_next].libPart.incorner.leng_s = DGGetItemValDouble (dialogID, EDITCONTROL_WIDTH_NEXT);
									placingZone.cells [xx][idxCell_next].libPart.incorner.wid_s = 0.100;
								} else if (idxCell_next > 0) {
									placingZone.cells [xx][idxCell_next].libPart.incorner.leng_s = 0.100;
									placingZone.cells [xx][idxCell_next].libPart.incorner.wid_s = DGGetItemValDouble (dialogID, EDITCONTROL_WIDTH_NEXT);
								}
								placingZone.cells [xx][idxCell_next].horLen = DGGetItemValDouble (dialogID, EDITCONTROL_WIDTH_NEXT);

								// ����
								placingZone.cells [xx][idxCell_next].libPart.incorner.hei_s = DGGetItemValDouble (dialogID, EDITCONTROL_HEIGHT_NEXT);
								placingZone.cells [xx][idxCell_next].verLen = DGGetItemValDouble (dialogID, EDITCONTROL_HEIGHT_NEXT);
							} else if (DGPopUpGetSelected (dialogID, POPUP_OBJ_TYPE_NEXT) == EUROFORM + 1) {
								placingZone.cells [xx][idxCell_next].objType = EUROFORM;

								// �԰���
								if (DGGetItemValLong (dialogID, CHECKBOX_SET_STANDARD_NEXT) == TRUE)
									placingZone.cells [xx][idxCell_next].libPart.form.eu_stan_onoff = true;
								else
									placingZone.cells [xx][idxCell_next].libPart.form.eu_stan_onoff = false;

								if (DGGetItemValLong (dialogID, CHECKBOX_SET_STANDARD_NEXT) == TRUE) {
									// �ʺ�
									placingZone.cells [xx][idxCell_next].libPart.form.eu_wid = atof (DGPopUpGetItemText (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS_NEXT, DGPopUpGetSelected (dialogID, POPUP_EUROFORM_WIDTH_OPTIONS_NEXT)).ToCStr ()) / 1000.0;
									placingZone.cells [xx][idxCell_next].horLen = placingZone.cells [xx][idxCell_next].libPart.form.eu_wid;
									// ����
									placingZone.cells [xx][idxCell_next].libPart.form.eu_hei = atof (DGPopUpGetItemText (dialogID, POPUP_EUROFORM_HEIGHT_OPTIONS_NEXT, DGPopUpGetSelected (dialogID, POPUP_EUROFORM_HEIGHT_OPTIONS_NEXT)).ToCStr ()) / 1000.0;
									placingZone.cells [xx][idxCell_next].verLen = placingZone.cells [xx][idxCell_next].libPart.form.eu_hei;
								} else {
									// �ʺ�
									placingZone.cells [xx][idxCell_next].libPart.form.eu_wid2 = DGGetItemValDouble (dialogID, EDITCONTROL_EUROFORM_WIDTH_OPTIONS_NEXT);
									placingZone.cells [xx][idxCell_next].horLen = placingZone.cells [xx][idxCell_next].libPart.form.eu_wid2;
									// ����
									placingZone.cells [xx][idxCell_next].libPart.form.eu_hei2 = DGGetItemValDouble (dialogID, EDITCONTROL_EUROFORM_HEIGHT_OPTIONS_NEXT);
									placingZone.cells [xx][idxCell_next].verLen = placingZone.cells [xx][idxCell_next].libPart.form.eu_hei2;
								}

								// ��ġ����
								if (DGGetItemValLong (dialogID, RADIO_ORIENTATION_1_EUROFORM_NEXT) == TRUE)
									placingZone.cells [xx][idxCell_next].libPart.form.u_ins_wall = true;
								else if (DGGetItemValLong (dialogID, RADIO_ORIENTATION_1_EUROFORM_NEXT) == FALSE) {
									placingZone.cells [xx][idxCell_next].libPart.form.u_ins_wall = false;
									// ����, ���� ���� ��ȯ
									temp = placingZone.cells [xx][idxCell_next].horLen;
									placingZone.cells [xx][idxCell_next].horLen = placingZone.cells [xx][idxCell_next].verLen;
									placingZone.cells [xx][idxCell_next].verLen = temp;
								}
							} else if (DGPopUpGetSelected (dialogID, POPUP_OBJ_TYPE_NEXT) == FILLERSPACER + 1) {
								placingZone.cells [xx][idxCell_next].objType = FILLERSPACER;

								// �ʺ�
								placingZone.cells [xx][idxCell_next].libPart.fillersp.f_thk = DGGetItemValDouble (dialogID, EDITCONTROL_WIDTH_NEXT);
								placingZone.cells [xx][idxCell_next].horLen = DGGetItemValDouble (dialogID, EDITCONTROL_WIDTH_NEXT);

								// ����
								placingZone.cells [xx][idxCell_next].libPart.fillersp.f_leng = DGGetItemValDouble (dialogID, EDITCONTROL_HEIGHT_NEXT);
								placingZone.cells [xx][idxCell_next].verLen = DGGetItemValDouble (dialogID, EDITCONTROL_HEIGHT_NEXT);
							} else if (DGPopUpGetSelected (dialogID, POPUP_OBJ_TYPE_NEXT) == PLYWOOD + 1) {
								placingZone.cells [xx][idxCell_next].objType = PLYWOOD;

								// �ʺ�
								placingZone.cells [xx][idxCell_next].libPart.plywood.p_wid = DGGetItemValDouble (dialogID, EDITCONTROL_WIDTH_NEXT);
								placingZone.cells [xx][idxCell_next].horLen = DGGetItemValDouble (dialogID, EDITCONTROL_WIDTH_NEXT);

								// ����
								placingZone.cells [xx][idxCell_next].libPart.plywood.p_leng = DGGetItemValDouble (dialogID, EDITCONTROL_HEIGHT_NEXT);
								placingZone.cells [xx][idxCell_next].verLen = DGGetItemValDouble (dialogID, EDITCONTROL_HEIGHT_NEXT);

								// ��ġ����
								if (DGGetItemValLong (dialogID, RADIO_ORIENTATION_1_PLYWOOD_NEXT) == TRUE)
									placingZone.cells [xx][idxCell_next].libPart.plywood.w_dir_wall = true;
								else if (DGGetItemValLong (dialogID, RADIO_ORIENTATION_1_PLYWOOD_NEXT) == FALSE){
									placingZone.cells [xx][idxCell_next].libPart.plywood.w_dir_wall = false;
									// ����, ���� ���� ��ȯ
									temp = placingZone.cells [xx][idxCell_next].horLen;
									placingZone.cells [xx][idxCell_next].horLen = placingZone.cells [xx][idxCell_next].verLen;
									placingZone.cells [xx][idxCell_next].verLen = temp;
								}
							} else if (DGPopUpGetSelected (dialogID, POPUP_OBJ_TYPE_NEXT) == WOOD + 1) {
								placingZone.cells [xx][idxCell_next].objType = WOOD;

								// �ʺ�
								placingZone.cells [xx][idxCell_next].libPart.wood.w_h = DGGetItemValDouble (dialogID, EDITCONTROL_WIDTH_NEXT);
								placingZone.cells [xx][idxCell_next].horLen = DGGetItemValDouble (dialogID, EDITCONTROL_WIDTH_NEXT);

								// ����
								placingZone.cells [xx][idxCell_next].libPart.wood.w_leng = DGGetItemValDouble (dialogID, EDITCONTROL_HEIGHT_NEXT);
								placingZone.cells [xx][idxCell_next].verLen = DGGetItemValDouble (dialogID, EDITCONTROL_HEIGHT_NEXT);

								// �β�
								placingZone.cells [xx][idxCell_next].libPart.wood.w_w = DGGetItemValDouble (dialogID, EDITCONTROL_THK_NEXT);

								// ����: 90��
								placingZone.cells [xx][idxCell_next].libPart.wood.w_ang = DegreeToRad (90.0);
							}
						}
					}

					break;
				case DG_CANCEL:
					break;
			}
		case DG_MSG_CLOSE:
			switch (item) {
				case DG_CLOSEBOX:
					break;
			}
	}

	result = item;

	return	result;
}