// ChildView.cpp : CChildView 클래스의 구현
//

#include "pch.h"
#include "framework.h"
#include "MAP1.h"        
#include "ChildView.h"
#include "Resource.h"    

#include <cmath>
#include <limits>
#include <queue>
#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define NODE_RADIUS 6
const double INF = 1e9;

CChildView::CChildView()
{
    // 1. 변수 초기화
    m_iSelectedNode = -1;
    m_iStartNode = -1;
    m_iEndNode = -1;
    m_bImageLoaded = false;


    
    
    //  PNG/JPG 호환
  
    HRSRC hRes = ::FindResource(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDB_PNG1), _T("PNG"));

    if (hRes)
    {
        DWORD dwSize = ::SizeofResource(AfxGetInstanceHandle(), hRes);
        HGLOBAL hGlob = ::LoadResource(AfxGetInstanceHandle(), hRes);
        LPVOID pData = ::LockResource(hGlob);

        if (pData && dwSize > 0)
        {
            // 메모리에 리소스 복사
            HGLOBAL hMem = ::GlobalAlloc(GMEM_MOVEABLE, dwSize);
            if (hMem)
            {
                LPVOID pMem = ::GlobalLock(hMem);
                if (pMem)
                {
                    memcpy(pMem, pData, dwSize);
                    ::GlobalUnlock(hMem);

                    // 스트림 생성 후 CImage에 로드
                    IStream* pStream = NULL;
                    if (::CreateStreamOnHGlobal(hMem, TRUE, &pStream) == S_OK)
                    {
                        m_bgImage.Load(pStream);
                        pStream->Release();

                        if (!m_bgImage.IsNull()) {
                            m_bImageLoaded = true;
                        }
                    }
                }
            }
        }
    }
    
}


CChildView::~CChildView()
{
}

BEGIN_MESSAGE_MAP(CChildView, CWnd)
    ON_WM_PAINT()
    ON_WM_LBUTTONDOWN()
    ON_WM_RBUTTONDOWN()
END_MESSAGE_MAP()

// CChildView 메시지 처리기

BOOL CChildView::PreCreateWindow(CREATESTRUCT& cs)
{
    if (!CWnd::PreCreateWindow(cs))
        return FALSE;

    cs.dwExStyle |= WS_EX_CLIENTEDGE;
    cs.style &= ~WS_BORDER;
    cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS,
        ::LoadCursor(nullptr, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1), nullptr);

    return TRUE;
}

void CChildView::OnPaint()
{
    CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트

    // 1. 배경 그리기
    if (m_bImageLoaded) {
        CRect rect;
        GetClientRect(&rect);
        // 이미지를 뷰 크기에 맞춰 늘려서 그림
        m_bgImage.Draw(dc.m_hDC, 0, 0, rect.Width(), rect.Height());
    }
    else {
        // 이미지가 없으면 흰색 배경
        CRect rect;
        GetClientRect(&rect);
        dc.FillSolidRect(rect, RGB(255, 255, 255));
    }

    // 2. 펜/브러시 설정
    CPen penLine(PS_SOLID, 2, RGB(0, 0, 255));      // 파란색 펜
    CPen penPath(PS_SOLID, 4, RGB(255, 0, 0));      // 빨간색 펜 (두껍게)
    CBrush brushNode(RGB(0, 0, 255));               // 파란색 브러시
    CBrush brushSelected(RGB(255, 0, 0));           // 빨간색 브러시

    dc.SetBkMode(TRANSPARENT);

    // 3. 엣지(선) 그리기
    for (const auto& edge : m_edges) {
        CPoint p1 = m_nodes[edge.u].pt;
        CPoint p2 = m_nodes[edge.v].pt;

        bool isPathEdge = false;
        // 최단 경로 엣지인지 확인
        if (m_pathResult.size() > 1) {
            for (size_t i = 0; i < m_pathResult.size() - 1; ++i) {
                int u = m_pathResult[i];
                int v = m_pathResult[i + 1];
                if ((edge.u == u && edge.v == v) || (edge.u == v && edge.v == u)) {
                    isPathEdge = true;
                    break;
                }
            }
        }

        CPen* pOldPen = dc.SelectObject(isPathEdge ? &penPath : &penLine);
        dc.MoveTo(p1);
        dc.LineTo(p2);
        dc.SelectObject(pOldPen);

        // 거리 텍스트 표시
        CString strDist;
        strDist.Format(_T("%.0f"), edge.weight);
        dc.TextOutW((p1.x + p2.x) / 2, (p1.y + p2.y) / 2, strDist);
    }

    // 4. 노드(점) 그리기
    for (int i = 0; i < m_nodes.size(); ++i) {
        CPoint pt = m_nodes[i].pt;
        bool isHighlight = (i == m_iStartNode || i == m_iEndNode || i == m_iSelectedNode);

        CBrush* pOldBrush = dc.SelectObject(isHighlight ? &brushSelected : &brushNode);
        dc.Ellipse(pt.x - NODE_RADIUS, pt.y - NODE_RADIUS, pt.x + NODE_RADIUS, pt.y + NODE_RADIUS);
        dc.SelectObject(pOldBrush);
    }
}

// [2] 우클릭: 노드 추가
void CChildView::OnRButtonDown(UINT nFlags, CPoint point)
{
    Node newNode;
    newNode.pt = point;
    newNode.id = (int)m_nodes.size();
    m_nodes.push_back(newNode);

    Invalidate(); // 화면 갱신
    CWnd::OnRButtonDown(nFlags, point);
}

// [3] 좌클릭: 엣지 연결(Ctrl) / 경로 탐색(Alt)

void CChildView::OnLButtonDown(UINT nFlags, CPoint point)
{
    int clickedIdx = GetClickedNodeIndex(point);

    if (clickedIdx != -1) {

        // 1. Ctrl 키가 눌려있는지 확인 (엣지 연결)
        if (GetKeyState(VK_CONTROL) < 0) {
            if (m_iSelectedNode == -1) {
                m_iSelectedNode = clickedIdx;
            }
            else if (m_iSelectedNode != clickedIdx) {
                // 거리 계산
                double dx = m_nodes[m_iSelectedNode].pt.x - m_nodes[clickedIdx].pt.x;
                double dy = m_nodes[m_iSelectedNode].pt.y - m_nodes[clickedIdx].pt.y;
                double dist = sqrt(dx * dx + dy * dy);

                Edge newEdge;
                newEdge.u = m_iSelectedNode;
                newEdge.v = clickedIdx;
                newEdge.weight = dist;
                m_edges.push_back(newEdge);

                m_iSelectedNode = -1;
            }
            else {
                m_iSelectedNode = -1; // 취소
            }
            Invalidate();
        }
        // 2. Alt 키가 눌려있는지 확인 (최단 경로 탐색)
        else if (GetKeyState(VK_MENU) < 0) {
            if (m_iStartNode == -1) {
                // 첫 번째 클릭: 출발점 설정
                m_pathResult.clear();
                m_iEndNode = -1;
                m_iStartNode = clickedIdx;
            }
            else {
                // 두 번째 클릭: 도착점 설정 및 알고리즘 실행
                m_iEndNode = clickedIdx;
                RunDijkstra(); // 여기서 메시지 박스
                m_iStartNode = -1;
            }
            Invalidate();
        }
    }

    CWnd::OnLButtonDown(nFlags, point);
}

// [4] 다익스트라 알고리즘

void CChildView::RunDijkstra()
{
    if (m_iStartNode == -1 || m_iEndNode == -1) return;

    int n = (int)m_nodes.size();
    std::vector<double> dist(n, INF);
    std::vector<int> parent(n, -1);

    std::priority_queue<std::pair<double, int>,
        std::vector<std::pair<double, int>>,
        std::greater<std::pair<double, int>>> pq;

    dist[m_iStartNode] = 0;
    pq.push({ 0, m_iStartNode });

    while (!pq.empty()) {
        double d = pq.top().first;
        int curr = pq.top().second;
        pq.pop();

        if (d > dist[curr]) continue;
        if (curr == m_iEndNode) break;

        for (const auto& edge : m_edges) {
            int next = -1;
            if (edge.u == curr) next = edge.v;
            else if (edge.v == curr) next = edge.u;

            if (next != -1) {
                if (dist[next] > dist[curr] + edge.weight) {
                    dist[next] = dist[curr] + edge.weight;
                    parent[next] = curr;
                    pq.push({ dist[next], next });
                }
            }
        }
    }

    // 결과 처리
    m_pathResult.clear();
    int curr = m_iEndNode;

    if (dist[curr] != INF) {
        // 1. 경로 데이터 저장
        while (curr != -1) {
            m_pathResult.push_back(curr);
            curr = parent[curr];
        }

        Invalidate();  
        UpdateWindow(); 

        // 2. 그 다음 메시지 박스 띄우기
        CString strMsg;
        strMsg.Format(_T("선택한 두 점 사이의 최단 거리 = %.0f"), dist[m_iEndNode]);
        MessageBox(strMsg, _T("Path"), MB_ICONWARNING | MB_OK);
    }
    else {
        MessageBox(_T("두 점이 연결되어 있지 않습니다."), _T("경로 없음"), MB_ICONERROR | MB_OK);
    }
}
int CChildView::GetClickedNodeIndex(CPoint pt)
{
    for (int i = 0; i < m_nodes.size(); ++i) {
        if (abs(pt.x - m_nodes[i].pt.x) < NODE_RADIUS * 2 &&
            abs(pt.y - m_nodes[i].pt.y) < NODE_RADIUS * 2) {
            return i;
        }
    }
    return -1;
}