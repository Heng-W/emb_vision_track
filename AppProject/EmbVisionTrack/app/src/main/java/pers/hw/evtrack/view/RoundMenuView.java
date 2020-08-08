package pers.hw.evtrack.view;


import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Color;
import android.graphics.RectF;
import android.graphics.Path;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;
import android.content.res.TypedArray;

import pers.hw.evtrack.R;


public class RoundMenuView extends View {

    private Paint paint = new Paint();

    private float centerX, centerY;
    private float centerRadius;
    private float radius;
    private int selectIdx = -1;

    private float centerRadiusCoff;
    private boolean hasCenter;
    private int menuCnt;
    private int angleStart;
    private float strokeWidth;
    private int strokeColor;
    private int fillColor;
    private int selectColor;
    private int centerFillColor;
    private int centerStrokeColor;

    private int markColor;
    private float markLCoff;
    private float markSCoff;


    private OnMenuClickListener menuClickListener;

    public void setOnMenuClickListener(OnMenuClickListener listener) {
        this.menuClickListener = listener;
    }

    public interface OnMenuClickListener {

        void onClick(RoundMenuView view, int idx);
    }


    public RoundMenuView(Context context) {
        this(context, null);

    }
    public RoundMenuView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);

    }

    public RoundMenuView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        final TypedArray array = context.getTheme().obtainStyledAttributes(attrs, R.styleable.RoundMenuView, defStyle, 0);
        hasCenter = array.getBoolean(R.styleable.RoundMenuView_hasCenter, true);
        menuCnt = array.getInteger(R.styleable.RoundMenuView_menuCnt, 4);
        angleStart = array.getInteger(R.styleable.RoundMenuView_angleStart, -135);
        angleStart %= 360;
        if (angleStart < 0)
            angleStart += 360;
        if (hasCenter) {
            centerRadiusCoff = array.getFloat(R.styleable.RoundMenuView_centerRadiusCoff, 0.3f);
        } else {
            centerRadiusCoff = 0;
        }
        if (centerRadiusCoff > 1) centerRadiusCoff = 1;
        else if (centerRadiusCoff < 0) centerRadiusCoff = 0;
        strokeWidth = array.getFloat(R.styleable.RoundMenuView_strokeWidth, 4);

        fillColor = array.getColor(R.styleable.RoundMenuView_fillColor, Color.WHITE);
        strokeColor = array.getColor(R.styleable.RoundMenuView_strokeColor, Color.BLACK);
        selectColor = array.getColor(R.styleable.RoundMenuView_selectColor, Color.GRAY);
        centerFillColor = array.getColor(R.styleable.RoundMenuView_centerFillColor, fillColor);
        centerStrokeColor = array.getColor(R.styleable.RoundMenuView_centerStrokeColor, strokeColor);

        markColor = array.getColor(R.styleable.RoundMenuView_markColor, Color.WHITE);
        markLCoff = array.getFloat(R.styleable.RoundMenuView_markLCoff, 0.6f);
        markSCoff = array.getFloat(R.styleable.RoundMenuView_markSCoff, 0.4f);

    }


    @Override
    protected void onDraw(Canvas canvas) {
        if (getWidth() < strokeWidth || getHeight() < strokeWidth)
            return;

        centerX = (float)getWidth() / 2;
        centerY = (float)getHeight() / 2;
        radius = Math.min(centerX, centerY) - strokeWidth / 2;
        centerRadius =  radius * centerRadiusCoff;//计算中心圆圈半径
        RectF oval = new RectF(centerX - radius, centerY - radius, centerX + radius, centerY + radius);

        paint.setAntiAlias(true);
        paint.setStyle(Paint.Style.FILL);
        paint.setColor(fillColor);

        canvas.drawArc(oval, 0, 360, true, paint);

        if (selectIdx >= 0) {
            paint.setColor(selectColor);
            canvas.drawArc(oval, (float)(angleStart + 360.0 / menuCnt * selectIdx), (float)(360 / menuCnt), true, paint);
        }

        paint.setColor(strokeColor);
        paint.setStyle(Paint.Style.STROKE);//空心
        paint.setStrokeWidth(strokeWidth);

        for (int i = 0; i < menuCnt; i++) {
            canvas.drawArc(oval, (float)(angleStart + 360.0 / menuCnt * i), (float)(360.0 / menuCnt), true, paint);
        }

        paint.setColor(markColor);
        paint.setStrokeWidth(strokeWidth);

        Path path = new Path();

        for (int i = 0; i < menuCnt; i++) {
            double angleRef = angleStart + 360.0 / menuCnt * i + 180.0 / menuCnt;
            double rl = centerRadius + (radius - centerRadius) * markLCoff;
            double rs = centerRadius + (radius - centerRadius) * markSCoff;
            double angleOfs = Math.asin(Math.sin(Math.PI / 4) * rl / rs);
            angleOfs = angleOfs * 180 / Math.PI + 90;
            angleOfs = 180 - 45 - angleOfs;

            float xRef = centerX + (float)(rl * Math.cos(angleRef / 180 * Math.PI));
            float yRef = centerY + (float)(rl * Math.sin(angleRef / 180 * Math.PI));

            float x1 = centerX + (float)(rs * Math.cos((angleRef - angleOfs) / 180 * Math.PI));
            float y1 = centerY + (float)(rs * Math.sin((angleRef - angleOfs) / 180 * Math.PI));

            float x2 = centerX + (float)(rs * Math.cos((angleRef + angleOfs) / 180 * Math.PI));
            float y2 = centerY + (float)(rs * Math.sin((angleRef + angleOfs) / 180 * Math.PI));

            path.moveTo(x1, y1);
            path.lineTo(xRef, yRef);
            path.lineTo(x2, y2);

            canvas.drawPath(path, paint);
        }

        if (hasCenter) {
            paint.setColor(centerFillColor);
            paint.setStyle(Paint.Style.FILL);

            oval = new RectF(centerX - centerRadius, centerY - centerRadius, centerX + centerRadius, centerY + centerRadius);
            canvas.drawArc(oval, 0, 360, false, paint);

            paint.setColor(centerStrokeColor);
            paint.setStyle(Paint.Style.STROKE);//空心
            canvas.drawArc(oval, 0, 360, false, paint);
        }

    }


    @Override
    public boolean onTouchEvent(MotionEvent e) {

        switch (e.getAction()) {
            case MotionEvent.ACTION_DOWN:
                selectIdx = getPointAreaIdx(e.getX(), e.getY());
                if (selectIdx >= 0) {
                    if (menuClickListener != null)
                        menuClickListener.onClick(this, selectIdx);
                }
                break;
            case MotionEvent.ACTION_UP:
                selectIdx = -1;
                break;
            default:
                break;
        }
        invalidate();
        //返回true表明处理方法已经处理该事务
        return true;

    }



    private int getPointAreaIdx(float x, float y) {
        double dist = Math.sqrt(Math.pow(x - centerX, 2) + Math.pow(y - centerY, 2));
        if (dist > radius || dist < centerRadius) return -1;
        float angle = getPointAngle(x, y);
        if (angle < angleStart) {
            angle += 360;
        }
        return (int)((angle - angleStart) / (360.0 / menuCnt));

    }

    public float getPointAngle(float x, float y) {
        float angle = 0;
        if (Math.abs(x - centerX) < 0.001) {
            if (y > centerY) angle = 90;
            else angle = 270;
        } else {
            double k = (y - centerY) / (x - centerX);
            angle = (float)(Math.atan(k) / Math.PI * 180);
            if (x < centerX)
                angle += 180;
            if (x > centerX && y < centerY)
                angle += 360;
        }

        return angle;

    }


}
