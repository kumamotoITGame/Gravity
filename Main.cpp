#include "DxLib.h"
#include <vector>
#include <cstdlib>
#include <ctime>

using namespace std;

// + 演算子を定義
VECTOR operator+( const VECTOR &a , const VECTOR &b ){
    return { a.x + b.x, a.y + b.y, a.z + b.z };
}

// 空中床の構造体
struct Floor{
    float x;
    float y;
    float width;
};

int WINAPI WinMain( HINSTANCE hInstance , HINSTANCE hPrevInstance , LPSTR lpCmdLine , int nCmdShow ){

    const int SCREEN_W = 800, SCREEN_H = 600;
    SetGraphMode( SCREEN_W , SCREEN_H , 32 );
    ChangeWindowMode( TRUE );

    if( DxLib_Init() == -1 ) return -1;
    SetDrawScreen( DX_SCREEN_BACK );

    //ステージの変化が欲しいときは0を変える
    srand( (unsigned int)0 );

    // キーの状態
    char key[ 256 ];
    char oldKey[ 256 ] = { 0 };

    // プレイヤー
    VECTOR v3Position = VGet( SCREEN_W / 2 , 0.0f , 0.0f );
    VECTOR v3Velocity = VGet( 0.0f , 0.0f , 0.0f );     // これを使うことで 物理的な動き（重力による落下、ジャンプ、摩擦による減速など） を計算できる

    // プレイヤー情報
    const int radius = 20;
    const float gravity = 0.6f;
    const float jumpVelocity = -12.0f;

    // カメラ用変数
    float cameraY = 0.0f;

    // 足場生成（初期足場）
    vector<Floor> floors;
    //floors.push_back( Floor{ 0.0f , SCREEN_H + 100 , SCREEN_W });// 絶対落ちない床
    for( int i = 0; i < 10; i++ ){
        Floor f;
        f.y = SCREEN_H - i * 110; // 高さごと
        f.width = 120 + rand() % 100;
        f.x = rand() % ( SCREEN_W - (int)f.width );
        floors.push_back( f );
    }

    while( 1 ){

        ClearDrawScreen();
        GetHitKeyStateAll( key );                       // キーボードの状態を一括で取得

        // 入力
        v3Velocity.x *= 0.85f;                          // 水平ベクトル移動成分の減衰 ( 今回はフロートなので掛け算を使用 )
        if( CheckHitKey( KEY_INPUT_LEFT ) )  v3Velocity.x = -4.0f;
        if( CheckHitKey( KEY_INPUT_RIGHT ) ) v3Velocity.x = 4.0f;

        // 重力 ( ゲームエンジンⅡの 簡易力学用変数 )
        v3Velocity.y += gravity;                        // UI ウィンドウは 上下 が +-逆になる

        // 移動 ( 下三行のどれか動くものを使用 )
        v3Position = VAdd( v3Position , v3Velocity );   // DXライブラリの通常使用
        //v3Position += v3Velocity;                     // DXライブラリの拡張（※DXライブラリの更新で消されてしまう)

        /*
        // ベクトルデータ型 改造例
        typedef struct tagVECTOR
        {
	        float					x, y, z ;

            //ここを追加記述
	        tagVECTOR &operator+=( const tagVECTOR &v ){
		        x += v.x;
		        y += v.y;
		        z += v.z;
		        return *this;
	        }

        } VECTOR, *LPVECTOR, FLOAT3, *LPFLOAT3 ;
        //v3Position = v3Velocity + v3Velocity ;        // 演算子を定義した場合
        */

        // 足場判定
        bool isGround = false;
        float oldPosY = v3Position.y - v3Velocity.y;    // 前のフレームでは何処にいたのかをv3Velocityで逆算
        for( auto &f : floors ){

            //床の線上を前と今で 貫通 したか ?
            if( 
                v3Velocity.y >= 0.0f &&                 // 下方向（落下中）
                oldPosY + radius <= f.y &&              // 前フレーム位置が床より上
                v3Position.y + radius > f.y &&          // 現在位置が床より下(前と今でサンド関係にあるか)
                v3Position.x >= f.x &&
                v3Position.x <= f.x + f.width 
                ){
                v3Position.y = f.y - radius;            // 床の上に位置修正
                //この場所でv3Velocityを見ながら着地音をならすと最適
                v3Velocity.y *= -0.25f;
                isGround = true;
            }
            else
            //貫通していないが y軸の静止状態で10ピクセル幅のBox 床判定 にいるか？
            if(
                v3Velocity.y >= 0.0f &&                 // 下方向（落下中）
                v3Position.y + radius >= f.y &&         // 現在位置が床より下にめり込んでいるか
                v3Position.y + radius <= f.y + 10.0f && // 過去の位置が床よりより上にいたか(10は消してもある程度動くがフロートの保険をとってる)
                v3Position.x >= f.x &&                  // プレイヤーのX座標が床の左端より右にあるか
                v3Position.x <= f.x + f.width           // プレイヤーのX座標が床の右端より左にあるか
            ){
                v3Position.y = f.y - radius;            // 床の上に位置修正
                v3Velocity.y = 0.0f;
                isGround = true;
            }
        }

        // ジャンプ
        if( key[ KEY_INPUT_SPACE ] == 1 && oldKey[ KEY_INPUT_SPACE ] == 0 ){
            if( isGround ){
                v3Velocity.y = jumpVelocity;            // v3Velocity経由でジャンプの力を渡す
            }
        }

        // カメラ追従
        cameraY = v3Position.y - SCREEN_H / 2;

        // 足場の描画
        for( auto &f : floors ){
            DrawLine( (int)f.x , (int)( f.y - cameraY ) ,
                      (int)( f.x + f.width ) , (int)( f.y - cameraY ) ,
                      GetColor( 255 , 255 , 255 ) , 4 );
        }

        // プレイヤー
        DrawCircle( (int)v3Position.x , (int)( v3Position.y - cameraY ) , radius , GetColor( 255 , 0 , 0 ) , TRUE );
        // プレイヤー当たり判定
        DrawCircle( (int)v3Position.x , (int)( v3Position.y - cameraY + radius ) , 2 , GetColor( 0 , 255 , 0 ) , TRUE );

        // 前フレームキー保存
        memcpy( oldKey , key , sizeof( key ) );

        ::ScreenFlip();
        //WaitTimer( 160 );

        if( ProcessMessage() != 0 )break;

        // Escapeキーで終了 ( 今回はすでに全部のキーをつかんでいるので配列で見る )
        if( key[ KEY_INPUT_ESCAPE ] ){
            break; // ループを抜けて終了
        }
    }

    ::DxLib_End();
    return 0;
}
