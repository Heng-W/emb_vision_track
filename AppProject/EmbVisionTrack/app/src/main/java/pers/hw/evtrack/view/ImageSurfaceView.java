package pers.hw.evtrack.view;


import android.view.View;
import android.view.GestureDetector;
import android.view.GestureDetector.SimpleOnGestureListener;
import android.view.MotionEvent;
import android.graphics.drawable.*;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.content.Context;
import android.util.AttributeSet;


public class ImageSurfaceView extends View {

    private int drawFlag = -1;

    private float xpos, ypos;
    private float width, height;

    private float startx, starty;
    private float endx, endy;

    private Paint paint = new Paint();
    //private GestureDetector gestureDetector;

    private OnEventListener eventListener;

    public void setOnEventListener(OnEventListener listener) {
        this.eventListener = listener;
    }

    public interface OnEventListener {
        public void onTouchOver(float x, float y, float w, float h);
    }


    public ImageSurfaceView(Context context) {
        this(context, null);
    }

    public ImageSurfaceView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public ImageSurfaceView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init();
    }

    private void init() {
        paint.setColor(Color.RED);
        paint.setStyle(Paint.Style.STROKE);//空心
        paint.setStrokeWidth(5);
        //gestureDetector = new GestureDetector(getContext(), new PaintEventListener());
    }


    public void showResult(float x, float y, float w, float h) {
        xpos = x * getWidth();
        ypos = y * getHeight();
        width = w * getWidth();
        height = h * getHeight();
        invalidate();
    }

    public boolean isDrawEnabled() {
        return drawFlag == 0;
    }

    public void enableDraw() {
        drawFlag = 0;

    }

    public void disableDraw() {
        drawFlag = -1;
        invalidate();
    }


    @Override
    public boolean onTouchEvent(MotionEvent e) {

        getParent().requestDisallowInterceptTouchEvent(true);

        if (drawFlag == -1) {
            return true;
        }

        switch (e.getAction()) {
            case MotionEvent.ACTION_DOWN:
                if (drawFlag != 0) break;
                drawFlag = 1;
                startx = e.getX();
                starty = e.getY();

                break;
            case MotionEvent.ACTION_MOVE:
                endx = e.getX();
                endy = e.getY();

                if (endx < 0) endx = 0;
                else if (endx > getWidth()) endx = getWidth();

                if (endy < 0) endy = 0;
                else if (endy > getHeight()) endy = getHeight();

                break;
            case MotionEvent.ACTION_UP:

                if (drawFlag != 1) break;
                drawFlag = 0;

                if (eventListener != null) {
                    float x = Math.min(startx, endx) / getWidth();
                    float y = Math.min(starty, endy) / getHeight();
                    float w = Math.abs(startx - endx) / getWidth();
                    float h = Math.abs(starty - endy) / getHeight();

                    eventListener.onTouchOver(x, y, w, h);
                }
                break;
            default:
                break;
        }
        invalidate();
        //返回true表明处理方法已经处理该事务
        return true;
    }


    @Override
    protected void onDraw(Canvas canvas) {

        paint.setColor(Color.RED);
        canvas.drawRect(xpos, ypos, xpos + width, ypos + height, paint);

        if (drawFlag == 1) {
            paint.setColor(Color.BLUE);
            canvas.drawRect(startx, starty, endx, endy, paint);

        }
    }

}


