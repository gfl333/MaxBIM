#ifndef	__BEAM_EUROFORM_PLACER__
#define __BEAM_EUROFORM_PLACER__

#include "MaxBIM.hpp"

namespace beamPlacerDG {
	// ��ü Ÿ��
	enum	libPartObjType {
		NONE,			// ����
		EUROFORM,		// ������v2.0
		FILLERSPACER,	// �ٷ������̼�v1.0
		PLYWOOD,		// ����v1.0
		WOOD,			// ����v1.0
		OUTCORNER_ANGLE	// �ƿ��ڳʾޱ�v1.0
	};

	// �����Ǵ� �� ��ġ
	enum	attachedSide {
		BOTTOM_SIDE,
		LEFT_SIDE,
		RIGHT_SIDE
	};

	// ���̾�α� �׸� �ε���
	enum	idxItems_1_forBeamPlacer {
		LABEL_BEAM_SECTION		= 3,
		SEPARATOR_1_BEAM_SECTION,
		LABEL_BEAM_HEIGHT,
		EDITCONTROL_BEAM_HEIGHT,
		LABEL_BEAM_WIDTH,
		EDITCONTROL_BEAM_WIDTH,
		
		SEPARATOR_2_LEFT,
		SEPARATOR_3_BOTTOM,
		SEPARATOR_4_RIGHT,
		EDITCONTROL_GAP_SIDE1,
		EDITCONTROL_GAP_BOTTOM,
		EDITCONTROL_GAP_SIDE2,

		LABEL_TOTAL_HEIGHT,
		EDITCONTROL_TOTAL_HEIGHT,
		LABEL_TOTAL_WIDTH,
		EDITCONTROL_TOTAL_WIDTH,

		LABEL_REST_SIDE,
		CHECKBOX_WOOD_SIDE,
		CHECKBOX_T_FORM_SIDE,
		CHECKBOX_FILLER_SIDE,
		CHECKBOX_B_FORM_SIDE,

		EDITCONTROL_REST_SIDE,
		EDITCONTROL_WOOD_SIDE,
		POPUP_T_FORM_SIDE,
		EDITCONTROL_FILLER_SIDE,
		POPUP_B_FORM_SIDE,

		CHECKBOX_L_FORM_BOTTOM,
		CHECKBOX_FILLER_BOTTOM,
		CHECKBOX_R_FORM_BOTTOM,

		POPUP_L_FORM_BOTTOM,
		EDITCONTROL_FILLER_BOTTOM,
		POPUP_R_FORM_BOTTOM,

		ICON_LAYER,
		LABEL_LAYER_SETTINGS,
		LABEL_LAYER_EUROFORM,
		LABEL_LAYER_FILLERSPACER,
		LABEL_LAYER_PLYWOOD,
		LABEL_LAYER_WOOD,
		LABEL_LAYER_OUTCORNER_ANGLE,

		USERCONTROL_LAYER_EUROFORM,
		USERCONTROL_LAYER_FILLERSPACER,
		USERCONTROL_LAYER_PLYWOOD,
		USERCONTROL_LAYER_WOOD,
		USERCONTROL_LAYER_OUTCORNER_ANGLE
	};

	enum	idxItems_2_forBeamPlacer {
		//
	};
}

// �� ���� ����
struct InfoBeam
{
	API_Guid	guid;	// ���� GUID

	short	floorInd;	// �� �ε���
	double	height;		// �� ����
	double	width;		// �� �ʺ�
	double	offset;		// �� �߽����κ��� ���� ���۷��� ������ �������Դϴ�.
	double	level;		// �ٴ� ������ ���� ���� ���ʸ� �����Դϴ�.

	API_Coord	begC;	// �� ���� ��ǥ
	API_Coord	endC;	// �� �� ��ǥ
};

// ���� ���� ����
struct InfoMorphForBeam
{
	API_Guid	guid;		// ������ GUID
	short		floorInd;	// �� �ε���
	double		level;		// ������ ����
};

// �׸��� �� �� ����
struct CellForBeam
{
	short		objType;	// enum libPartObjType ����

	API_Guid	guid;		// ��ü�� GUID

	double	leftBottomX;	// ���ϴ� ��ǥ X
	double	leftBottomY;	// ���ϴ� ��ǥ Y
	double	leftBottomZ;	// ���ϴ� ��ǥ Z
	double	ang;			// ȸ�� ���� (����: Radian, ȸ����: Z��)

	double	dirLen;			// �� ��ġ���� ����
	double	perLen;			// �� �������� ����
	short	attached_side;	// �Ϻ�(BOTTOM_SIDE), ������(LEFT_SIDE), ������(RIGHT_SIDE)

	union {
		Euroform		form;
		Plywood			plywood;
		FillerSpacer	fillersp;
		Wood			wood;
		OutcornerAngle	outangle;
	} libPart;
};

// �� ���� ����
struct BeamPlacingZone
{
	// ���� ���� ��ǥ�� ����, �� ��ǥ�� �������̶�� ������

	// �� ���� ����
	double	level;				// �� ���� ����
	double	ang;				// ȸ�� ���� (����: Radian, ȸ����: Z��)
	double	areaHeight;			// ���� ����

	API_Coord3D		begC;		// ��ġ ���� ������
	API_Coord3D		endC;		// ��ġ ���� ����
	double	beamLength;			// ���� �� ��ü ����

	// ���� �� ���� ����
	bool	bInterfereBeam;				// ���� �� ����
	double	posInterfereBeamFromLeft;	// ���� �� ��ġ (�� ���� ��ǥ-����-�κ���)
	double	interfereBeamWidth;			// ���� �� �ʺ�
	double	interfereBeamHeight;		// ���� �� ����

	// �� �糡 ����
	double	marginBeginAtSide;				// ���� ���� �κ� ����
	double	marginEndAtSide;				// ���� �� �κ� ����
	double	marginBeginAtBottom;			// �Ϻ� ���� �κ� ����
	double	marginEndAtBottom;				// �Ϻ� �� �κ� ����
	double	marginBeginAtSide_updated;		// ���� ���� �κ� ���� (������Ʈ ��)
	double	marginEndAtSide_updated;		// ���� �� �κ� ���� (������Ʈ ��)
	double	marginBeginAtBottom_updated;	// �Ϻ� ���� �κ� ���� (������Ʈ ��)
	double	marginEndAtBottom_updated;		// �Ϻ� �� �κ� ���� (������Ʈ ��)

	// �� �糡 ���� ä���� ����
	bool	bFillMarginBeginAtSide;			// ���� ���� �κ� ���� ä��
	bool	bFillMarginEndAtSide;			// ���� �� �κ� ���� ä��
	bool	bFillMarginBeginAtBottom;		// �Ϻ� ���� �κ� ���� ä��
	bool	bFillMarginEndAtBottom;			// �Ϻ� �� �κ� ���� ä��

	// �� ���� (����)
	CellForBeam		cellsFromBeginAtLSide [4][20];	// ���� �κк��� �ٿ����� �� (����)
	CellForBeam		cellsFromBeginAtRSide [4][20];	// ���� �κк��� �ٿ����� �� (������)
	CellForBeam		cellsFromEndAtLSide [4][20];	// �� �κк��� �ٿ����� �� (����)
	CellForBeam		cellsFromEndAtRSide [4][20];	// �� �κк��� �ٿ����� �� (������)
	CellForBeam		cellCenterAtLSide [4];			// ��� �κп� ���̴� �� (����)
	CellForBeam		cellCenterAtRSide [4];			// ��� �κп� ���̴� �� (������)
	short			nCellsFromBeginAtSide;			// ������ ���� �κ� �� ����
	short			nCellsFromEndAtSide;			// ������ �� �κ� �� ����

	// �� ���� (�Ϻ�)
	CellForBeam		cellsFromBeginAtBottom [3][20];	// ���� �κк��� �ٿ����� ��
	CellForBeam		cellsFromEndAtBottom [3][20];	// �� �κк��� �ٿ����� ��
	CellForBeam		cellCenterAtBottom [3];			// ��� �κп� ���̴� ��
	short			nCellsFromBeginAtBottom;		// �Ϻ��� ���� �κ� �� ����
	short			nCellsFromEndAtBottom;			// �Ϻ��� �� �κ� �� ����

	double			gapSide;			// ������ ���� (����)
	double			gapBottom;			// ������ ���� (�Ϻ�)
};

// ������ �� ��ġ �Լ�
GSErrCode	placeEuroformOnBeam (void);				// 3�� �޴�: ���� �������� ��ġ�ϴ� ���� ��ƾ
void		initCellsForBeam (BeamPlacingZone* placingZone);				// Cell �迭�� �ʱ�ȭ��
void		addNewColFromBeginAtSide (BeamPlacingZone* target_zone);		// ���� ���� �κ� - ���ο� ���� �߰��� (�� �ϳ��� �ø��� �߰��� ���� ������ �� ���� ����)
void		delLastColFromBeginAtSide (BeamPlacingZone* target_zone);		// ���� ���� �κ� - ������ ���� ������
void		addNewColFromEndAtSide (BeamPlacingZone* target_zone);			// ���� �� �κ� - ���ο� ���� �߰��� (�� �ϳ��� �ø��� �߰��� ���� ������ �� ���� ����)
void		delLastColFromEndAtSide (BeamPlacingZone* target_zone);			// ���� �� �κ� - ������ ���� ������
void		addNewColFromBeginAtBottom (BeamPlacingZone* target_zone);		// �Ϻ� ���� �κ� - ���ο� ���� �߰��� (�� �ϳ��� �ø��� �߰��� ���� ������ �� ���� ����)
void		delLastColFromBeginAtBottom (BeamPlacingZone* target_zone);		// �Ϻ� ���� �κ� - ������ ���� ������
void		addNewColFromEndAtBottom (BeamPlacingZone* target_zone);		// �Ϻ� �� �κ� - ���ο� ���� �߰��� (�� �ϳ��� �ø��� �߰��� ���� ������ �� ���� ����)
void		delLastColFromEndAtBottom (BeamPlacingZone* target_zone);		// �Ϻ� �� �κ� - ������ ���� ������
void		alignPlacingZoneForBeam (BeamPlacingZone* target_zone);			// Cell ������ ����ʿ� ���� ����ȭ�� ��ġ�� ��������
API_Guid	placeLibPartForBeam (CellForBeam objInfo);						// �ش� �� ������ ������� ���̺귯�� ��ġ
GSErrCode	fillRestAreasForBeam (void);									// ������/�ٷ�/���縦 ä�� �� ������ ���� ä��� (������ ����/���� �� �ƿ��ڳʾޱ�)
short DGCALLBACK beamPlacerHandler1 (short message, short dialogID, short item, DGUserData userData, DGMessageData msgData);	// 1�� ��ġ�� ���� ���Ǹ� ��û�ϴ� 1�� ���̾�α�
short DGCALLBACK beamPlacerHandler2 (short message, short dialogID, short item, DGUserData userData, DGMessageData msgData);	// 1�� ��ġ �� ������ ��û�ϴ� 2�� ���̾�α�
short DGCALLBACK beamPlacerHandler3 (short message, short dialogID, short item, DGUserData userData, DGMessageData msgData);	// 2�� ���̾�α׿��� �� ���� ��ü Ÿ���� �����ϱ� ���� 3�� ���̾�α�

#endif