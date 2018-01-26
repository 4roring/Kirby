#include "stdafx.h"
#include "Kirby.h"
#include "Eff_Dash.h"
#include "Eff_Normal_FlyAtt.h"
#include "Eff_NormalAtt.h"
#include "Eff_ShootingStar.h"

CKirby::CKirby()
{
}


CKirby::~CKirby()
{
}

void CKirby::Initialize()
{
	// 상태 초기화
	m_iMaxHp = 60;
	m_iHp = m_iMaxHp;

	//m_tInfo = { 50, 400, 96, 105 };
	m_tInfo.fCX = 96;
	m_tInfo.fCY = 105;
	m_iHitBoxCX = 40;
	m_iHitBoxCY = 40;

	m_fImageX = 0.f;
	m_fImageY = -9.f;

	m_fSpeed = 3.8f;
	m_fJumpPow = 8.6f;

	m_fVelocityX = 0.f;
	m_fVelocityY = 0.f;
	m_fGravity = 0.0f;
	m_fDrag = 0.f;
	m_fAccX = 0.65f;
	m_fAccY = 1.2f;

	m_bFlipX = true; // Flip의 디폴트는 왼쪽(-방향)
	m_bJump = false;
	m_bIsGround = false;
	m_bDash = false;
	m_bAttack = false;

	m_iInputFrame = g_iFrame;
	m_iAttSquence = 0;

	// 애니메이션 초기화
	m_pFrameKey = TEXT("Normal_Right");
	m_eCurState = IDLE;
	m_ePreState = m_eCurState;

	m_tFrame.iStart = 0;
	m_tFrame.iEnd = 34;
	m_tFrame.iScene = 0;
	m_tFrame.dwTime = GetTickCount();
	m_tFrame.dwSpeed = 100;

	m_iAtt = 10;
}

void CKirby::LateInit()
{
}

OBJ_STATE CKirby::Update()
{
	if (m_eCurState == DAMAGE) return PLAY;
	if (m_bEat) return PLAY;

	if (!m_bAttack)
	{
		Input();
		Slide();
	}

	Move();
	Jump();
	Attack();

	m_tInfo.fX += m_fVelocityX;

	return PLAY;
}

void CKirby::LateUpdate()
{
	if (m_eCurState == DAMAGE) isDamage();

	FrameMove();

	KirbyUpdateRect();

	m_bIsGround = CCollision::PlayerToGround(this, GameManager->GetObjList(OBJ_GROUND));

	if (m_tInfo.fY + m_iHitBoxCY * 0.5f < 0)
		m_tInfo.fY = 0;

	if (m_tInfo.fX + m_iHitBoxCX *0.5f < 0)
		m_tInfo.fX = 0;

	NoDamageState();
	Eat();
	SceneChange();
	ScrollMove();
}

void CKirby::Render(HDC hDC)
{
	if(GameManager->GetDebugMode())
		DrawHitBox(hDC);

	if (m_bEat)
		DrawAlphaBlack(hDC, 100);

	if(!m_bNoDamage)
		DrawObject(hDC, m_pFrameKey);
	else if(m_bNoDamage && g_iFrame % 2 == 0)
		DrawObject(hDC, m_pFrameKey);
}

void CKirby::Release()
{
}

void CKirby::ApplyDamage(int iDamage)
{
	if (!m_bNoDamage)
	{
		CActor::ApplyDamage(iDamage);
		m_eCurState = DAMAGE;
		m_bNoDamage = true;
	}

	if (m_bSlide)
		m_bNoDamage = true;
}

void CKirby::Input()
{
	if (m_bSlide) return;

	if (InputManager->KeyDown(VK_UP))
		CCollision::InterectionDoor(this, GameManager->GetObjList(OBJ_INTERECTION));

	if (m_fVelocityX == 0 && m_fVelocityY == 0 && !m_bSlide && !m_bJump && !m_bAttack)
	{
		if(m_bInhail)
			m_eCurState = INHAILIDLE;
		else
			m_eCurState = IDLE;
	}

	if (m_eCurState == INHAILIDLE && m_bInhail && !m_bAttack)
	{
		if (m_tFrame.iStart == 1)
			m_tFrame.dwSpeed = 100;
		else
			m_tFrame.dwSpeed = 2000;
	}

	// 7프레임 안에 연타시 대시 입력
	if (InputManager->KeyDown(VK_LEFT) && m_iInputFrame > g_iFrame && !m_bFly)
	{
		m_bFlipX = false;
		m_bDash = true;
		CreateDashEffect();
	}
	else if (InputManager->KeyUp(VK_LEFT) && m_iInputFrame < g_iFrame && !m_bFly)
	{
		m_iInputFrame = g_iFrame + 7;
		m_bDash = false;
	}

	if (InputManager->KeyDown(VK_RIGHT) && m_iInputFrame > g_iFrame && !m_bFly)
	{
		m_bFlipX = true;
		m_bDash = true;
		CreateDashEffect();
	}
	else if (InputManager->KeyUp(VK_RIGHT) && m_iInputFrame < g_iFrame && !m_bFly)
	{
		m_iInputFrame = g_iFrame + 7;
		m_bDash = false;
	}

	if (InputManager->Key(VK_DOWN) && !m_bNoDamage)
	{
		m_iHitBoxCY = 15;

		if (m_bInhail)
		{
			m_bEat = true;
			m_bInhail = false;
		}
		else if (!m_bSlide && !m_bFly && !m_bInhail)
		{
			m_eCurState = DOWN;
			m_bDash = false;
			if (m_tFrame.iStart == 1)
				m_tFrame.dwSpeed = 100;
			else
				m_tFrame.dwSpeed = 2000;
		}

		// 슬라이딩
		if (InputManager->KeyDown('Z') && !m_bJump && !m_bFly
			|| InputManager->KeyDown('X') && !m_bJump && !m_bFly)
		{
			m_bSlide = true;
			CreateDashEffect();

			m_iAtt = 20;
		}
	}
	else if (InputManager->KeyDown('Z'))
	{
		if (m_bIsGround && !m_bJump)
		{
			m_bIsGround = false;
			m_bJump = true;
			m_fVelocityY = 1.f;
		}
		else if (m_bJump && !m_bInhail)
			m_bFly = true;
	}
	else
		m_iHitBoxCY = 40;
}

// 이동
void CKirby::Move()
{
	if (m_bSlide) return;

	if (m_bDash && !m_bAttack)
	{
		m_fAccX = 1.f;
		m_fSpeed = 5.1f;

		if (!m_bJump && !m_bFly && !m_bAttack)
			m_eCurState = DASH;

		if (m_tFrame.iStart == m_tFrame.iEnd)
			m_tFrame.iStart = 1;
	}
	else if (m_bJump)
	{
		m_fAccX = 2.f;
		m_fSpeed = 5.6f;
	}
	else if (!m_bSlide)
	{
		m_fAccX = 0.65f;
		m_fSpeed = 3.8f;
	}

	if (InputManager->Key(VK_LEFT))
	{
		if (m_bFly)
			m_fVelocityX = -3.f;
		else if (m_eCurState != DOWN && !m_bAttack)
			m_fVelocityX -= m_fAccX;

		m_bFlipX = false;
		m_pFrameKey = TEXT("Normal_Left");
	}
	else if (InputManager->Key(VK_RIGHT))
	{
		if (m_bFly)
			m_fVelocityX = 3.f;
		else if (m_eCurState != DOWN && !m_bAttack)
			m_fVelocityX += m_fAccX;

		m_bFlipX = true;
		m_pFrameKey = TEXT("Normal_Right");
	}

	// 최대 속도를 넘지 않음
	if (m_fVelocityX > m_fSpeed && m_fVelocityX > 0)
		m_fVelocityX = m_fSpeed;
	else if (m_fVelocityX < -m_fSpeed && m_fVelocityX < 0)
		m_fVelocityX = -m_fSpeed;

	//m_fVelocityX -= m_fVelocityX * 0.09f;

	m_fVelocityX -= m_fVelocityX * 0.1f;

	if (-0.1f < m_fVelocityX && m_fVelocityX < 0.2f)
	{
		m_fVelocityX = 0.f;
		m_fSpeed = 3.8f;
	}

	if (m_fVelocityX != 0.f && m_eCurState != DOWN && !m_bDash
		&& !m_bJump && !m_bFly && m_eCurState != FLYATTACK && !m_bAttack)
		m_eCurState = MOVE;
}

void CKirby::Attack()
{
	// 공격 입력
	if (InputManager->KeyDown('X') && !m_bSlide)
	{
		if (m_bFly)
		{
			m_bFly = false;
			m_eCurState = FLYATTACK;
		}
		else if (m_bInhail)
			m_eCurState = SHOOTSTAR;
		else
		{
			m_eCurState = ATTACK;
			m_iAttSquence = 0;
		}
		m_bAttack = true;
	}

	if (m_bAttack && m_eCurState == FLYATTACK)
	{
		m_eCurState = FLYATTACK;
		if (m_tFrame.iStart == 2)
		{
			++m_tFrame.iStart;
			float fX = m_bFlipX ? 50.f : -50.f;
			GameManager->AddObject(CAbsFactory<CEff_Normal_FlyAtt>::CreateObject(m_tInfo.fX + fX, m_tInfo.fY, m_bFlipX), PLAYER_ATT);
		}
		if (m_tFrame.iStart == m_tFrame.iEnd)
		{
			m_bAttack = false;
			m_eCurState = IDLE;
		}
	}
	else if (m_bAttack && m_eCurState == SHOOTSTAR)
	{
		m_eCurState = SHOOTSTAR;
		if (m_tFrame.iStart == 0)
		{
			GameManager->AddObject(CAbsFactory<CEff_ShootingStar>::CreateObject(m_tInfo.fX, m_tInfo.fY - 10.f, m_bFlipX), PLAYER_ATT);
			++m_tFrame.iStart;
		}
			
		if (m_tFrame.iStart == m_tFrame.iEnd)
		{
			m_bAttack = false;
			m_bInhail = false;
			m_eCurState = IDLE;
		}
	}
	else if (InputManager->Key('X') && m_bAttack)
	{
		// 흡수
		if (m_bInhail)
			m_eCurState = INHAIL;
		else
			m_eCurState = ATTACK;
		m_bDash = false;

		if (m_pTarget == nullptr && m_iAttSquence == 0)
		{
			GameManager->AddObject(CAbsFactory<CEff_NormalAtt>::CreateObject(this), OBJ_EFFECT);
			m_pTarget = GameManager->GetObjList(OBJ_EFFECT).back();
		}

		if (m_iAttSquence < 10)
		{
			if (m_tFrame.iStart == 7)
			{
				m_tFrame.iStart = 6;
				++m_iAttSquence;
			}
		}

		// TODO: 먹었을 때 조건 추가
		if (m_iAttSquence == 10)
		{
			if (m_pTarget)
			{
				m_pTarget->SetActive(false);
				m_pTarget = nullptr;
			}
		}

		if (m_bInhail)
			m_iAttSquence = 10;

		if (m_tFrame.iStart == m_tFrame.iEnd)
			m_bAttack = false;
	}
	else if (InputManager->KeyUp('X') && m_bAttack)
	{
		m_bAttack = false;
		m_iAttSquence = 0;
		if (m_pTarget)
		{
			m_pTarget->SetActive(false);
			m_pTarget = nullptr;
		}
	}
}

// 점프, 날기
// FIXME: 땅에서만 다닐때 중력의 영향을 안받음
void CKirby::Jump()
{
	if (InputManager->Key('Z') && m_bJump)
	{
		if (m_bFly)
		{
			m_fVelocityY = 3.5f;
			m_bDash = false;
			if (m_iInputFrame < g_iFrame)
			{
				m_fGravity = 0.f;
				m_iInputFrame = g_iFrame + 5;
			}
		}
		else
			m_fVelocityY += m_fAccY;

		if (m_fVelocityY > m_fJumpPow && !m_bFly)
			m_fVelocityY = m_fJumpPow;
	}

	if (m_bFly)
	{
		// 날고있을 때 고정 애니메이션
		m_eCurState = FLY;

		m_bJump = true;

		if (m_tFrame.iStart > 3)
			m_tFrame.dwSpeed = 50;

		if (m_tFrame.iStart == m_tFrame.iEnd)
			m_tFrame.iStart = 4;
	}
	else if (m_fVelocityY > 0 && m_bJump && !m_bAttack)
	{
		// 점프 중일 때에는 1프레임의 애니메이션을 길게 재생
		m_eCurState = JUMP;
		m_tFrame.dwSpeed = 2000;
	}
	else if (m_fVelocityY < 0 && m_bJump && !m_bInhail)
	{
		// 떨어질 때에는 특정 프레임 애니메이션만 재생

		if (!m_bAttack)
		{
			m_eCurState = JUMP;
			m_tFrame.dwSpeed = 40;
		}

		if (m_tFrame.iStart == m_tFrame.iEnd - 3)
			m_tFrame.dwSpeed = 2000;
	}

	// 날기 애니메이션
	if (!InputManager->Key('Z') && m_bFly)
	{
		if (m_tFrame.iStart > 3)
			m_tFrame.dwSpeed = 150;
		if (m_tFrame.iStart > 4)
			m_tFrame.iStart = 3;
	}

	if (m_bIsGround)
	{
		// 착지 애니메이션
		if (m_eCurState == JUMP && m_bJump && !m_bAttack)
		{
			m_tFrame.iStart = m_tFrame.iEnd - 2;
			m_tFrame.dwSpeed = 50;
		}
		m_fGravity = -0.2f;
		m_fVelocityY = m_fGravity;
		m_bJump = false;
	}
	else
	{
		m_fGravity += m_fAccY * 0.07f;

		if (m_fVelocityY < 0)
			m_fVelocityY -= m_fGravity * 0.5f;
		else
			m_fVelocityY -= m_fGravity;

		// 상태에 따른 낙하 속도 조절 (비행, 일반)
		if (m_fVelocityY < 0 && m_fVelocityY < -1.5f && m_bFly)
			m_fVelocityY = -3.f;
		else if (m_fVelocityY < 0 && m_fVelocityY < -6.5f)
			m_fVelocityY = -6.5f;

		m_tInfo.fY -= m_fVelocityY;
	}
}

// 태클
// 이동과 점프의 조합 로직
// 소형 적에게 데미지를 입힌다. 대형, 보스는 X
// 공격력이 20이된다. 자신은 데미지를 입지 않게 된다.
void CKirby::Slide()
{
	if (m_bSlide)
	{
		m_fSpeed = 10.8f;

		if (m_tFrame.iStart == 1)
			m_tFrame.dwSpeed = 2000;

		if (!m_bFlipX)
			m_fVelocityX -= 3.f;
		else
			m_fVelocityX += 3.f;

		m_fDrag += m_fAccX * 0.04f;

		if (m_fVelocityX > m_fSpeed && m_fVelocityX > 0)
			m_fVelocityX = m_fSpeed;
		else if (m_fVelocityX < -m_fSpeed && m_fVelocityX < 0)
			m_fVelocityX = -m_fSpeed;

		m_fVelocityX -= m_fVelocityX * m_fDrag;

		m_eCurState = SLIDE;

		if (-0.3f < m_fVelocityX && m_fVelocityX < 0.3f)
		{
			m_fDrag = 0.f;
			m_fVelocityX = 0.f;
			m_fSpeed = 3.8f;
			m_bSlide = false;
		}
	}
	else
		m_iAtt = 10;
}

void CKirby::SceneChange()
{
	if (m_ePreState != m_eCurState)
	{
		switch (m_eCurState)
		{
		case IDLE:
			m_tFrame.iStart = 0;
			m_tFrame.iEnd = 34;
			m_tFrame.iScene = 0;
			m_tFrame.dwTime = GetTickCount();
			m_tFrame.dwSpeed = 100;
			break;
		case INHAILIDLE:
			m_tFrame.iStart = 0;
			m_tFrame.iEnd = 1;
			m_tFrame.iScene = 14;
			m_tFrame.dwTime = GetTickCount();
			m_tFrame.dwSpeed = 600;
			break;
		case DOWN:
			m_tFrame.iStart = 0;
			m_tFrame.iEnd = 1;
			m_tFrame.iScene = 1;
			m_tFrame.dwTime = GetTickCount();
			m_tFrame.dwSpeed = 1000;
			break;
		case SLIDE:
			m_tFrame.iStart = 0;
			m_tFrame.iEnd = 1;
			m_tFrame.iScene = 2;
			m_tFrame.dwTime = GetTickCount();
			m_tFrame.dwSpeed = 100;
			break;
		case MOVE:
			if (m_bInhail)
			{
				m_tFrame.iStart = 0;
				m_tFrame.iEnd = 12;
				m_tFrame.iScene = 15;
				m_tFrame.dwTime = GetTickCount();
				m_tFrame.dwSpeed = 70;
			}
			else
			{
				m_tFrame.iStart = 0;
				m_tFrame.iEnd = 9;
				m_tFrame.iScene = 3;
				m_tFrame.dwTime = GetTickCount();
				m_tFrame.dwSpeed = 70;
			}

			break;
		case DASH:
			if (m_bInhail)
			{
				m_tFrame.iStart = 0;
				m_tFrame.iEnd = 12;
				m_tFrame.iScene = 15;
				m_tFrame.dwTime = GetTickCount();
				m_tFrame.dwSpeed = 40;
			}
			else
			{
				m_tFrame.iStart = 1;
				m_tFrame.iEnd = 7;
				m_tFrame.iScene = 5;
				m_tFrame.dwTime = GetTickCount();
				m_tFrame.dwSpeed = 40;
			}
			break;
		case JUMP:
			if (m_bInhail)
			{
				m_tFrame.iStart = 0;
				m_tFrame.iEnd = 4;
				m_tFrame.iScene = 22;
				m_tFrame.dwTime = GetTickCount();
				m_tFrame.dwSpeed = 100;
			}
			else
			{
				m_tFrame.iStart = 0;
				m_tFrame.iEnd = 11;
				m_tFrame.iScene = 6;
				m_tFrame.dwTime = GetTickCount();
				m_tFrame.dwSpeed = 100;
			}
			break;
		case FLY:
			m_tFrame.iStart = 0;
			m_tFrame.iEnd = 8;
			m_tFrame.iScene = 10;
			m_tFrame.dwTime = GetTickCount();
			m_tFrame.dwSpeed = 60;
			break;
		case FLYATTACK:
			m_tFrame.iStart = 2;
			m_tFrame.iEnd = 4;
			m_tFrame.iScene = 11;
			m_tFrame.dwTime = GetTickCount();
			m_tFrame.dwSpeed = 130;
			break;
		case ATTACK:
			m_tFrame.iStart = 0;
			m_tFrame.iEnd = 11;
			m_tFrame.iScene = 12;
			m_tFrame.dwTime = GetTickCount();
			m_tFrame.dwSpeed = 100;
			break;
		case INHAIL:
			m_tFrame.iStart = 0;
			m_tFrame.iEnd = 4;
			m_tFrame.iScene = 13;
			m_tFrame.dwTime = GetTickCount();
			m_tFrame.dwSpeed = 80;
			break;
		case DAMAGE:
			m_tFrame.iStart = 0;
			m_tFrame.iEnd = 7;
			m_tFrame.iScene = 18;
			m_tFrame.dwTime = GetTickCount();
			m_tFrame.dwSpeed = 50;
			break;
		case EAT:
			m_tFrame.iStart = 0;
			m_tFrame.iEnd = 5;
			m_tFrame.iScene = 16;
			m_tFrame.dwTime = GetTickCount();
			m_tFrame.dwSpeed = 60;
			break;
		case SHOOTSTAR:
			m_tFrame.iStart = 0;
			m_tFrame.iEnd = 4;
			m_tFrame.iScene = 17;
			m_tFrame.dwTime = GetTickCount();
			m_tFrame.dwSpeed = 40;
			break;
		}
		m_ePreState = m_eCurState;
	}
}

void CKirby::ScrollMove()
{
	float fScrollX = GameManager->GetScrollX();

	if (WINCX * 0.5f < m_tInfo.fX + fScrollX && m_fVelocityX > 0)
		GameManager->SetScrollX(fScrollX - m_fVelocityX);

	if (WINCX * 0.5f > m_tInfo.fX + fScrollX && m_fVelocityX < 0)
		GameManager->SetScrollX(fScrollX - m_fVelocityX);
}

void CKirby::isDamage()
{
	m_fVelocityX = m_bFlipX ? -2.f : 2.f;

	if (m_bSlide)
	{
		m_fVelocityX = m_bFlipX ? -20.f : 20.f;
		m_fVelocityY = 5.f;
		m_bSlide = false;
	}
	else
	{
		m_fVelocityX = m_bFlipX ? -3.f : 3.f;
		m_fVelocityY -= 0.1f;
	}
		
	
	m_tInfo.fX += m_fVelocityX;
	m_tInfo.fY -= m_fVelocityY;

	if (m_tFrame.iStart == m_tFrame.iEnd)
	{
		m_eCurState = IDLE;
		m_bSlide = false;
	}
	
}

void CKirby::NoDamageState()
{
	if (m_eCurState == DAMAGE)
		m_dwDamageTime = GetTickCount() + 2000;

	if (m_dwDamageTime < GetTickCount())
		m_bNoDamage = false;
}

void CKirby::Eat()
{
	if(m_bEat)
	{
		m_eCurState = EAT;

		if (m_tFrame.iStart == m_tFrame.iEnd)
		{
			if (m_eInhailType == NORMAL)
				m_eInhailType = EMPTY;

			m_bEat = false;
			m_eCurState = IDLE;
		}
	}
}

void CKirby::KirbyUpdateRect()
{
	m_tRect.left = LONG(m_tInfo.fX + m_fImageX - m_tInfo.fCX / 2.f);
	m_tRect.top = LONG(m_tInfo.fY + m_fImageY - m_tInfo.fCY / 2.f);
	m_tRect.right = LONG(m_tInfo.fX + m_fImageX + m_tInfo.fCX / 2.f);
	m_tRect.bottom = LONG(m_tInfo.fY + m_fImageY + m_tInfo.fCY / 2.f);

	m_tHitBox.left = LONG(m_tInfo.fX - m_iHitBoxCX / 2.f);
	m_tHitBox.top = LONG(m_tInfo.fY - m_iHitBoxCY + 20.f);
	m_tHitBox.right = LONG(m_tInfo.fX + m_iHitBoxCX / 2.f);
	m_tHitBox.bottom = LONG(m_tInfo.fY + 20.f);
}

void CKirby::CreateDashEffect()
{
	GameManager->AddObject(CAbsFactory<CEff_Dash>::CreateObject(m_tInfo.fX, m_tInfo.fY - 10.f, m_bFlipX), OBJ_EFFECT);
}

void CKirby::DrawAlphaBlack(HDC hDC, int iAlpha)
{
	HDC hMemDC = BmpManager->GetMapBit()[TEXT("BackBlack")]->GetMemDC();

	BLENDFUNCTION bf = { 0, 0, (BYTE)iAlpha, 0 };

	GdiAlphaBlend(hDC, 0, 0, WINCX, WINCY, hMemDC, 0, 0, WINCX, WINCY, bf);
}