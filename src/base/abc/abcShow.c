/**CFile****************************************************************

  FileName    [abcShow.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Visualization procedures using DOT software and GSView.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcShow.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#ifdef WIN32
#include <process.h> 
#else
#include <unistd.h>
#endif

#include <ctype.h>
#include "abc.h"
#include "base/main/main.h"
#include "base/io/ioAbc.h"

#ifdef ABC_USE_CUDD
#include "bdd/extrab/extraBdd.h"
#endif

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern void Abc_ShowFile( char * FileNameDot, int fKeepDot );
static void Abc_ShowGetFileName( char * pName, char * pBuffer );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

#ifdef ABC_USE_CUDD

/**Function*************************************************************

  Synopsis    [Visualizes BDD of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NodeShowBddOne( DdManager * dd, DdNode * bFunc )
{
    char * FileNameDot = "temp.dot";
    FILE * pFile;
    if ( (pFile = fopen( FileNameDot, "w" )) == NULL )
    {
        fprintf( stdout, "Cannot open the intermediate file \"%s\".\n", FileNameDot );
        return;
    }
    Cudd_DumpDot( dd, 1, (DdNode **)&bFunc, NULL, NULL, pFile );
    fclose( pFile );
    Abc_ShowFile( FileNameDot, 0 );
}

/**Function*************************************************************

  Synopsis    [Visualizes BDD of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NodeShowBdd( Abc_Obj_t * pNode, int fCompl )
{
    FILE * pFile;
    Vec_Ptr_t * vNamesIn;
    char FileNameDot[200];
    char * pNameOut;
    DdManager * dd = (DdManager *)pNode->pNtk->pManFunc;

    assert( Abc_NtkIsBddLogic(pNode->pNtk) );
    // create the file name
    Abc_ShowGetFileName( Abc_ObjName(pNode), FileNameDot );
    // check that the file can be opened
    if ( (pFile = fopen( FileNameDot, "w" )) == NULL )
    {
        fprintf( stdout, "Cannot open the intermediate file \"%s\".\n", FileNameDot );
        return;
    }

    // set the node names 
    vNamesIn = Abc_NodeGetFaninNames( pNode );
    pNameOut = Abc_ObjName(pNode);
    if ( fCompl )
        Cudd_DumpDot( dd, 1, (DdNode **)&pNode->pData, (char **)vNamesIn->pArray, &pNameOut, pFile );
    else
    {
        DdNode * bAdd = Cudd_BddToAdd( dd, (DdNode *)pNode->pData ); Cudd_Ref( bAdd );
        Cudd_DumpDot( dd, 1, (DdNode **)&bAdd, (char **)vNamesIn->pArray, &pNameOut, pFile );
        Cudd_RecursiveDeref( dd, bAdd );
    }
    Abc_NodeFreeNames( vNamesIn );
    Abc_NtkCleanCopy( pNode->pNtk );
    fclose( pFile );

    // visualize the file 
    Abc_ShowFile( FileNameDot, 0 );
}
void Abc_NtkShowBdd( Abc_Ntk_t * pNtk, int fCompl, int fReorder )
{
    char FileNameDot[200];
    char ** ppNamesIn, ** ppNamesOut;
    DdManager * dd; DdNode * bFunc;
    Vec_Ptr_t * vFuncsGlob;
    Abc_Obj_t * pObj; int i;
    FILE * pFile;

    assert( Abc_NtkIsStrash(pNtk) );
    dd = (DdManager *)Abc_NtkBuildGlobalBdds( pNtk, 10000000, 1, fReorder, 0, 0 );
    if ( dd == NULL )
    {
        printf( "Construction of global BDDs has failed.\n" );
        return;
    }
    //printf( "Shared BDD size = %6d nodes.\n", Cudd_ReadKeys(dd) - Cudd_ReadDead(dd) );

    // complement the global functions
    vFuncsGlob = Vec_PtrAlloc( Abc_NtkCoNum(pNtk) );
    Abc_NtkForEachCo( pNtk, pObj, i )
        Vec_PtrPush( vFuncsGlob, Abc_ObjGlobalBdd(pObj) );

    // create the file name
    Abc_ShowGetFileName( pNtk->pName, FileNameDot );
    // check that the file can be opened
    if ( (pFile = fopen( FileNameDot, "w" )) == NULL )
    {
        fprintf( stdout, "Cannot open the intermediate file \"%s\".\n", FileNameDot );
        return;
    }

    // set the node names 
    ppNamesIn = Abc_NtkCollectCioNames( pNtk, 0 );
    ppNamesOut = Abc_NtkCollectCioNames( pNtk, 1 );
    if ( fCompl )
        Cudd_DumpDot( dd, Abc_NtkCoNum(pNtk), (DdNode **)Vec_PtrArray(vFuncsGlob), ppNamesIn, ppNamesOut, pFile );
    else
    {
        DdNode ** pbAdds = ABC_ALLOC( DdNode *, Vec_PtrSize(vFuncsGlob) );
        Vec_PtrForEachEntry( DdNode *, vFuncsGlob, bFunc, i )
            { pbAdds[i] = Cudd_BddToAdd( dd, bFunc ); Cudd_Ref( pbAdds[i] ); }
        Cudd_DumpDot( dd, Abc_NtkCoNum(pNtk), pbAdds, ppNamesIn, ppNamesOut, pFile );
        Vec_PtrForEachEntry( DdNode *, vFuncsGlob, bFunc, i )
            Cudd_RecursiveDeref( dd, pbAdds[i] );
        ABC_FREE( pbAdds );
    }
    ABC_FREE( ppNamesIn );
    ABC_FREE( ppNamesOut );
    fclose( pFile );

    // cleanup
    Abc_NtkFreeGlobalBdds( pNtk, 0 );
    Vec_PtrForEachEntry( DdNode *, vFuncsGlob, bFunc, i )
        Cudd_RecursiveDeref( dd, bFunc );
    Vec_PtrFree( vFuncsGlob );
    Extra_StopManager( dd );
    Abc_NtkCleanCopy( pNtk );

    // visualize the file 
    Abc_ShowFile( FileNameDot, 0 );
}

#else
void Abc_NodeShowBdd( Abc_Obj_t * pNode, int fCompl ) {}
void Abc_NtkShowBdd( Abc_Ntk_t * pNtk, int fCompl, int fReorder ) {}
#endif
typedef struct {
    int index;
    char * name;
} VarInfo;

/* ============================================================= */
/*        DFS遍历BDD，寻找第一个满足路径（避开S变量）             */
/* ============================================================= */
int DfsBddWithFilter(DdNode *node, int *cube, int nVars, VarInfo **pIndex2Var, st__table *visited, int *foundSolution)
{
    DdNode *T, *E;
    int index;
    
    // 如果已经找到解，直接返回
    if (*foundSolution) return 0;
    
    // 终端节点处理（0或1都是有效解）
    if (Cudd_IsConstant(node)) {
        if (!(*foundSolution)) {
            *foundSolution = 1;
            return 1; // 找到解，无论是常量0还是常量1
        }
        return 0;
    }
    
    // 重复访问检查
    if (st__is_member(visited, (char *)node) == 1) {
        return 0;  // 已访问过，剪枝
    }
    
    if (node == NULL)
        return -2;
        
    index = node->index;
    VarInfo *pInfo = pIndex2Var[index];
    if (pInfo == NULL) {
        return -1;
    }

    // 遇到变量名包含s或S，标记为已访问，直接回溯，不访问子树
    if (strchr(pInfo->name, 's') || strchr(pInfo->name, 'S')) {
        if (st__add_direct(visited, (char *)node, NULL) == st__OUT_OF_MEM) {
            printf("Memory error in visited table\n");
        }
        return 0; // 直接返回，剪枝整个子树
    }

    // 标记当前节点为已访问
    if (st__add_direct(visited, (char *)node, NULL) == st__OUT_OF_MEM) {
        printf("Memory error in visited table\n");
        return -1;
    }

    // 赋值1，递归Then分支
    cube[index] = 1;
    T = cuddT(node);
    if (DfsBddWithFilter(T, cube, nVars, pIndex2Var, visited, foundSolution) == 1)
        return 1; // 找到解就返回

    if (*foundSolution) return 1;

    // 赋值0，递归Else分支
    cube[index] = 0;
    E = Cudd_Regular(cuddE(node));
    if (DfsBddWithFilter(E, cube, nVars, pIndex2Var, visited, foundSolution) == 1)
        return 1; // 找到解就返回

    // 回溯
    cube[index] = -1;
    return 0;
}

/* ============================================================= */
/*        执行BDD DFS分析并输出JSON格式                          */
/* ============================================================= */
void bdd_xkd(Abc_Ntk_t* pNtk, int fVerbose)
{
    st__table *visited = NULL;
    DdManager *dd;
    DdNode *bFunc;
    Vec_Ptr_t *vFuncsGlob;
    VarInfo **pIndex2Var;
    Abc_Obj_t *pPi, *pObj;
    int i;

    // 构建全局BDD
    assert(Abc_NtkIsStrash(pNtk));
    dd = (DdManager *)Abc_NtkBuildGlobalBdds(pNtk, 10000000, 1, 1, 0, 0);
    if (dd == NULL) {
        printf("Construction of global BDDs has failed.\n");
        return;
    }

    // 构建映射index → VarInfo*
    pIndex2Var = ABC_CALLOC(VarInfo *, 100);  // 初始化全为 NULL
    Abc_NtkForEachPi(pNtk, pPi, i)
    {
        VarInfo *pInfo = ABC_ALLOC(VarInfo, 1);
        pInfo->index = i;
        pInfo->name = Abc_ObjName(pPi);
        pIndex2Var[i] = pInfo;  // 直接建立映射：index → VarInfo*
    }

    // 创建数组记录每个节点的值
    int nVars = Cudd_ReadSize(dd);
    // 遍历每个输出节点
    int n = Abc_NtkCoNum(pNtk);
    vFuncsGlob = Vec_PtrAlloc(Abc_NtkCoNum(pNtk));
    Abc_NtkForEachCo(pNtk, pObj, i)
        Vec_PtrPush(vFuncsGlob, Abc_ObjGlobalBdd(pObj));
    
    DdNode **pbAdds = ABC_ALLOC(DdNode *, Vec_PtrSize(vFuncsGlob));
    Vec_PtrForEachEntry(DdNode *, vFuncsGlob, bFunc, i)
    {
        pbAdds[i] = Cudd_BddToAdd(dd, bFunc);
        Cudd_Ref(pbAdds[i]);
    }
    
    // 输出JSON格式
    printf("{\n");
    printf("  \"input_file_name\":\"case01\",\n");
    printf("  \"cases\":[\n");
    
    for (i = 0; i < n; i++) {
        printf("    {\n");
        printf("      \"name\":\"D[%d]\",\n", i);
        
        int *cube = ABC_ALLOC(int, nVars + 10);
        memset(cube, -1, sizeof(int) * (nVars + 10));
        
        // 为每个输出创建新的visited表
        visited = st__init_table(st__ptrcmp, st__ptrhash);
        int foundSolution = 0;
        
        // 搜索第一个解
        DfsBddWithFilter(Cudd_Regular(pbAdds[i]), cube, nVars, pIndex2Var, visited, &foundSolution);
        
        if (foundSolution) {
            printf("      \"solution\":\"Yes\",\n");
            printf("      \"assignments\":{\n");
            
            // 只输出L变量
            int firstL = 1;
            for (int j = 0; j < nVars; j++) {
                VarInfo *pInfo = pIndex2Var[j];
                if (pInfo == NULL) continue;
                if (pInfo->name && (pInfo->name[0] == 'L' || pInfo->name[0] == 'l')) {
                    if (!firstL) printf(",\n");
                    int value = (cube[j] == -1) ? 0 : cube[j];  // 默认为0
                    
                    // 确保输出格式为L[n]，即使原名称可能只是"L"
                    char *name = pInfo->name;
                    if (strlen(name) == 1 && (name[0] == 'L' || name[0] == 'l')) {
                        // 如果名称只是"L"，输出为"L[0]"
                        printf("        \"L[0]\":%d", value);
                    } else {
                        // 如果已经有完整格式如"L[0]"、"L[1]"等，直接使用
                        printf("        \"%s\":%d", name, value);
                    }
                    firstL = 0;
                }
            }
            printf("\n      }\n");
        } else {
            printf("      \"solution\":\"No\"\n");
        }
        
        printf("    }");
        if (i < n - 1) printf(",");
        printf("\n");
        
        ABC_FREE(cube);
        st__free_table(visited); // 每个输出完成后清理visited表
    }
    
    printf("  ]\n");
    printf("}\n");

    // 清理内存
    for (i = 0; i < Vec_PtrSize(vFuncsGlob); i++) {
           if (pIndex2Var[i])
            ABC_FREE(pIndex2Var[i]);
    }
    ABC_FREE(pIndex2Var);
    Cudd_RecursiveDeref(dd, pbAdds[i]);
    
    ABC_FREE(pbAdds);
    Vec_PtrFree(vFuncsGlob);   
    //Extra_StopManager(dd);
}

    int nS;
    int nX;
    int nL;
void  orderlsx(Abc_Ntk_t * pNtk)
{
Vec_Ptr_t *vX = Vec_PtrAlloc(16);
    Vec_Ptr_t *vL = Vec_PtrAlloc(16);
    Vec_Ptr_t *vS = Vec_PtrAlloc(16);
    Vec_Ptr_t *vOther = Vec_PtrAlloc(16);
    Abc_Obj_t *pObj;
    int i;
    // Classify PIs by their first letter
    Abc_NtkForEachPi(pNtk, pObj, i)
    {
        char *pName = Abc_ObjName(pObj);
        if (pName == NULL)
        {
            Vec_PtrPush(vOther, pObj);
            continue;
        }
        char ch = toupper((unsigned char)pName[0]);
        if (ch == 'L')
            Vec_PtrPush(vL, pObj);
        else if (ch == 'S')
            Vec_PtrPush(vS, pObj);
        else if (ch == 'X')
            Vec_PtrPush(vX, pObj);

        else
            Vec_PtrPush(vOther, pObj);
    }
    // rebuild pNtk->vPis
    Vec_PtrClear(pNtk->vPis);
    Vec_PtrForEachEntry(Abc_Obj_t *, vL, pObj, i)
        Vec_PtrPush(pNtk->vPis, pObj);
    Vec_PtrForEachEntry(Abc_Obj_t *, vS, pObj, i)
        Vec_PtrPush(pNtk->vPis, pObj);
    Vec_PtrForEachEntry(Abc_Obj_t *, vX, pObj, i)
        Vec_PtrPush(pNtk->vPis, pObj);
    Vec_PtrForEachEntry(Abc_Obj_t *, vOther, pObj, i)
        Vec_PtrPush(pNtk->vPis, pObj);

    nS = Vec_PtrSize(vS);
    nX = Vec_PtrSize(vX);
    nL = Vec_PtrSize(vL);

    printf("PIs classified: S=%d, X=%d, L=%d\n", nS, nX, nL);
    Vec_PtrFree(vX);
    Vec_PtrFree(vL);
    Vec_PtrFree(vS);
}
/**Function*************************************************************

  Synopsis    [Visualizes a reconvergence driven cut at the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NodeShowCut( Abc_Obj_t * pNode, int nNodeSizeMax, int nConeSizeMax )
{
    FILE * pFile;
    char FileNameDot[200];
    Abc_ManCut_t * p;
    Vec_Ptr_t * vCutSmall;
    Vec_Ptr_t * vCutLarge;
    Vec_Ptr_t * vInside;
    Vec_Ptr_t * vNodesTfo;
    Abc_Obj_t * pTemp;
    int i;

    assert( Abc_NtkIsStrash(pNode->pNtk) );

    // start the cut computation manager
    p = Abc_NtkManCutStart( nNodeSizeMax, nConeSizeMax, 2, ABC_INFINITY );
    // get the recovergence driven cut
    vCutSmall = Abc_NodeFindCut( p, pNode, 1 );
    // get the containing cut
    vCutLarge = Abc_NtkManCutReadCutLarge( p );
    // get the array for the inside nodes
    vInside = Abc_NtkManCutReadVisited( p );
    // get the inside nodes of the containing cone
    Abc_NodeConeCollect( &pNode, 1, vCutLarge, vInside, 1 );

    // add the nodes in the TFO 
    vNodesTfo = Abc_NodeCollectTfoCands( p, pNode, vCutSmall, ABC_INFINITY );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodesTfo, pTemp, i )
        Vec_PtrPushUnique( vInside, pTemp );

    // create the file name
    Abc_ShowGetFileName( Abc_ObjName(pNode), FileNameDot );
    // check that the file can be opened
    if ( (pFile = fopen( FileNameDot, "w" )) == NULL )
    {
        fprintf( stdout, "Cannot open the intermediate file \"%s\".\n", FileNameDot );
        return;
    }
    // add the root node to the cone (for visualization)
    Vec_PtrPush( vCutSmall, pNode );
    // write the DOT file
    Io_WriteDotNtk( pNode->pNtk, vInside, vCutSmall, FileNameDot, 0, 0, 0 );
    // stop the cut computation manager
    Abc_NtkManCutStop( p );

    // visualize the file 
    Abc_ShowFile( FileNameDot, 0 );
}

/**Function*************************************************************

  Synopsis    [Visualizes AIG with choices.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkShow( Abc_Ntk_t * pNtk0, int fGateNames, int fSeq, int fUseReverse, int fKeepDot, int fAigIds )
{
    FILE * pFile;
    Abc_Ntk_t * pNtk;
    Abc_Obj_t * pNode;
    Vec_Ptr_t * vNodes;
    int nBarBufs;
    char FileNameDot[200];
    int i;

    assert( Abc_NtkIsStrash(pNtk0) || Abc_NtkIsLogic(pNtk0) );
    if ( Abc_NtkIsStrash(pNtk0) && Abc_NtkGetChoiceNum(pNtk0) )
    {
        printf( "Temporarily visualization of AIGs with choice nodes is disabled.\n" );
        return;
    }
    // create the file name
    Abc_ShowGetFileName( pNtk0->pName, FileNameDot );
    // check that the file can be opened
    if ( (pFile = fopen( FileNameDot, "w" )) == NULL )
    {
        fprintf( stdout, "Cannot open the intermediate file \"%s\".\n", FileNameDot );
        return;
    }
    fclose( pFile );


    // convert to logic SOP
    pNtk = Abc_NtkDup( pNtk0 );
    if ( Abc_NtkIsLogic(pNtk) && !Abc_NtkHasMapping(pNtk) )
        Abc_NtkToSop( pNtk, -1, ABC_INFINITY );

    // collect all nodes in the network
    vNodes = Vec_PtrAlloc( 100 );
    Abc_NtkForEachObj( pNtk, pNode, i )
        Vec_PtrPush( vNodes, pNode );
    // write the DOT file
    nBarBufs = pNtk->nBarBufs;
    pNtk->nBarBufs = 0;
    if ( fSeq )
        Io_WriteDotSeq( pNtk, vNodes, NULL, FileNameDot, fGateNames, fUseReverse );
    else
        Io_WriteDotNtk( pNtk, vNodes, NULL, FileNameDot, fGateNames, fUseReverse, fAigIds );
    pNtk->nBarBufs = nBarBufs;
    Vec_PtrFree( vNodes );

    // visualize the file 
    Abc_ShowFile( FileNameDot, fKeepDot );
    Abc_NtkDelete( pNtk );
}


/**Function*************************************************************

  Synopsis    [Shows the given DOT file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ShowFile( char * FileNameDot, int fKeepDot )
{
    FILE * pFile;
    char * FileGeneric;
    char FileNamePs[200];
    char CommandDot[1000];
    char * pDotName;
    char * pDotNameWin = "dot.exe";
    char * pDotNameUnix = "dot";
    char * pGsNameWin = "gsview32.exe";
    char * pGsNameUnix = "gv";
    int RetValue;

    // get DOT names from the resource file
    if ( Abc_FrameReadFlag("dotwin") )
        pDotNameWin = Abc_FrameReadFlag("dotwin");
    if ( Abc_FrameReadFlag("dotunix") )
        pDotNameUnix = Abc_FrameReadFlag("dotunix");

#ifdef WIN32
    pDotName = pDotNameWin;
#else
    pDotName = pDotNameUnix;
#endif

    // check if the input DOT file is okay
    if ( (pFile = fopen( FileNameDot, "r" )) == NULL )
    {
        fprintf( stdout, "Cannot open the intermediate file \"%s\".\n", FileNameDot );
        return;
    }
    fclose( pFile );

    // create the PostScript file name
    FileGeneric = Extra_FileNameGeneric( FileNameDot );
    sprintf( FileNamePs,  "%s.ps",  FileGeneric ); 
    ABC_FREE( FileGeneric );

    // generate the PostScript file using DOT
    sprintf( CommandDot,  "%s -Tps -o %s %s", pDotName, FileNamePs, FileNameDot ); 
#if defined(__wasm)
    RetValue = -1;
#else
    RetValue = system( CommandDot );
#endif
    if ( RetValue == -1 )
    {
        fprintf( stdout, "Command \"%s\" did not succeed.\n", CommandDot );
        return;
    }
    // check that the input PostScript file is okay
    if ( (pFile = fopen( FileNamePs, "r" )) == NULL )
    {
        fprintf( stdout, "Cannot open intermediate file \"%s\".\n", FileNamePs );
        return;
    }
    fclose( pFile ); 


    // get GSVIEW names from the resource file
    if ( Abc_FrameReadFlag("gsviewwin") )
        pGsNameWin = Abc_FrameReadFlag("gsviewwin");
    if ( Abc_FrameReadFlag("gsviewunix") )
        pGsNameUnix = Abc_FrameReadFlag("gsviewunix");

    // spawn the viewer
#ifdef WIN32
    if ( !fKeepDot ) _unlink( FileNameDot );
    if ( _spawnl( _P_NOWAIT, pGsNameWin, pGsNameWin, FileNamePs, NULL ) == -1 )
        if ( _spawnl( _P_NOWAIT, "C:\\Program Files\\Ghostgum\\gsview\\gsview32.exe", 
            "C:\\Program Files\\Ghostgum\\gsview\\gsview32.exe", FileNamePs, NULL ) == -1 )
            if ( _spawnl( _P_NOWAIT, "C:\\Program Files\\Ghostgum\\gsview\\gsview64.exe", 
                "C:\\Program Files\\Ghostgum\\gsview\\gsview64.exe", FileNamePs, NULL ) == -1 )
            {
                fprintf( stdout, "Cannot find \"%s\".\n", pGsNameWin );
                return;
            }
#else
    {
        char CommandPs[1000];
        if ( !fKeepDot ) unlink( FileNameDot );
        sprintf( CommandPs,  "%s %s &", pGsNameUnix, FileNamePs ); 
#if defined(__wasm)
        if ( 1 )
#else
        if ( system( CommandPs ) == -1 )
#endif
        {
            fprintf( stdout, "Cannot execute \"%s\".\n", CommandPs );
            return;
        }
    }
#endif
}

/**Function*************************************************************

  Synopsis    [Derives the DOT file name.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ShowGetFileName( char * pName, char * pBuffer )
{
    char * pCur;
    // creat the file name
    sprintf( pBuffer, "%s.dot", pName );
    // get rid of not-alpha-numeric characters
    for ( pCur = pBuffer; *pCur; pCur++ )
        if ( !((*pCur >= '0' && *pCur <= '9') || (*pCur >= 'a' && *pCur <= 'z') || 
               (*pCur >= 'A' && *pCur <= 'Z') || (*pCur == '.')) )
            *pCur = '_';
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkWriteFlopDependency( Abc_Ntk_t * pNtk, char * pFileName )
{
    FILE * pFile;
    Vec_Ptr_t * vSupp;
    Abc_Obj_t * pObj, * pTemp;
    int i, k, Count;
    pFile = fopen( pFileName, "w" );
    if ( pFile == NULL )
    {
        printf( "Cannot open input file %s.\n", pFileName );
        return;
    }
    fprintf( pFile, "# Flop dependency for \"%s\" generated by ABC on %s\n", Abc_NtkName(pNtk), Extra_TimeStamp() );
    fprintf( pFile, "digraph G {\n" );
    fprintf( pFile, "  graph [splines=true overlap=false];\n" );
    fprintf( pFile, "  size = \"7.5,10\";\n" );
    fprintf( pFile, "  center = true;\n" );
//    fprintf( pFile, "  edge [len=3,dir=forward];\n" );
    fprintf( pFile, "  edge [dir=forward];\n" );
    Abc_NtkForEachLatchInput( pNtk, pObj, i )
    {
        Abc_ObjFanout0( Abc_ObjFanout0(pObj) )->iTemp = i;
        vSupp = Abc_NtkNodeSupport( pNtk, &pObj, 1 );
        Count = 0;
        Vec_PtrForEachEntry( Abc_Obj_t *, vSupp, pTemp, k )
            Count += Abc_ObjIsPi(pTemp);
        Vec_PtrFree( vSupp );
        fprintf( pFile, "  { rank = same; %d [label=\"%d(%d)\"]; }\n", i, i, Count );
    }
    Abc_NtkForEachLatchInput( pNtk, pObj, i )
    {
        vSupp = Abc_NtkNodeSupport( pNtk, &pObj, 1 );
        Count = 0;
        Vec_PtrForEachEntry( Abc_Obj_t *, vSupp, pTemp, k )
            if ( !Abc_ObjIsPi(pTemp) )
                fprintf( pFile, "  %4d -> %4d\n", pTemp->iTemp, i );
        Vec_PtrFree( vSupp );
    }
    fprintf( pFile, "}\n" );
    fclose( pFile );
}


/**Function*************************************************************

  Synopsis    [Visualizes AIG with choices.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkShowFlopDependency( Abc_Ntk_t * pNtk )
{
    FILE * pFile;
    char FileNameDot[200];
    assert( Abc_NtkIsStrash(pNtk) || Abc_NtkIsLogic(pNtk) );
    // create the file name
    Abc_ShowGetFileName( pNtk->pName, FileNameDot );
    // check that the file can be opened
    if ( (pFile = fopen( FileNameDot, "w" )) == NULL )
    {
        fprintf( stdout, "Cannot open the intermediate file \"%s\".\n", FileNameDot );
        return;
    }
    fclose( pFile );
    // write the DOT file
    Abc_NtkWriteFlopDependency( pNtk, FileNameDot );
    // visualize the file 
    Abc_ShowFile( FileNameDot, 0 );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

