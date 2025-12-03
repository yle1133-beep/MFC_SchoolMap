// ChildView.h : CChildView 클래스의 인터페이스
//

#pragma once
#include <vector>
#include <atlimage.h> // 이미지 처리를 위해 필수

struct Node {
    CPoint pt;          // 좌표
    int id;             // 인덱스
};

struct Edge {
    int u, v;           // 연결된 두 노드
    double weight;      // 거리
};

// CChildView 창
class CChildView : public CWnd
{
    // 생성입니다.
public:
    CChildView();

    // 특성입니다.
public:
    std::vector<Node> m_nodes;
    std::vector<Edge> m_edges;

    int m_iSelectedNode;
    int m_iStartNode;
    int m_iEndNode;

    std::vector<int> m_pathResult;

    CImage m_bgImage;               // 배경 이미지
    bool m_bImageLoaded;

// 작업입니다.
public:
    int GetClickedNodeIndex(CPoint pt);      // 헬퍼 함수
    void RunDijkstra();                      // 알고리즘 함수

    // 재정의입니다.
protected:
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

    // 구현입니다.
public:
    virtual ~CChildView();

    // 생성된 메시지 맵 함수
protected:
    afx_msg void OnPaint();
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
    DECLARE_MESSAGE_MAP()
};