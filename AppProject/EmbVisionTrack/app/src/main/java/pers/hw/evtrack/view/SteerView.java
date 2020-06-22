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
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import android.view.View.*;

import pers.hw.evtrack.R;


public class SteerView extends View {

    private Paint paint = new Paint();

    private float centerX, centerY;
    private float startAngle;
    private float referAngle;
    private float lastReferAngle;
    private float endAngle;

    private OnTouchListener touchListener;

    public void setOnTouchListener(OnTouchListener listener) {
        this.touchListener = listener;
    }

    public interface OnTouchListener {

        public void onMove(SteerView view, float angle);
    }


    public SteerView(Context context) {
        this(context, null);

    }
	
    public SteerView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);

    }
	
    public SteerView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);

    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        final int width = getSize(widthMeasureSpec);
        final int height = getSize(heightMeasureSpec);
        setMeasuredDimension(width, height);
    }
    /**
     * 获取测量大小
     */
    private int getSize(int measureSpec) {
        int result = 300;
        int specMode = MeasureSpec.getMode(measureSpec);
        int specSize = MeasureSpec.getSize(measureSpec);
        if (specMode == MeasureSpec.EXACTLY) {
            result = specSize;
        } else if (specMode == MeasureSpec.AT_MOST) {

            result = Math.min(result, specSize);
        }
        return result;
    }


    @Override
    protected void onDraw(Canvas canvas) {

        centerX = (float)getWidth() / 2;
        centerY = (float)getHeight() / 2;

        int strokeWidth = 10;
        float radius = Math.min(centerX, centerY) - strokeWidth / 2;
        float centerRadius = (float)(radius * 0.3);

        paint.setAntiAlias(true);

        paint.setColor(Color.BLACK);
        paint.setStyle(Paint.Style.STROKE);//空心
        paint.setStrokeWidth(strokeWidth);

        Path path = new Path();
        // path.moveTo(centerX-radius,centerY);
        path.addCircle(centerX, centerY, radius, Path.Direction.CW);
       
        float x1 = (float)(centerX - 0.99 * radius);
        float y1 = (float)(centerY - 0.06 * radius);

        float x2 = 2 * centerX - x1;
        float y2 = 2 * centerY - y1;

        path.moveTo(x1, y1);
        path.quadTo(centerX, (float)(centerY - centerRadius * 1.6), x2, y1);

        float refX = (float)(centerX - radius * 0.47);
        float refY = (float)(centerY - radius * 0.21);

        path.moveTo(refX, refY);
        path.quadTo(centerX, (float)(centerY + centerRadius * 2.2), 2 * centerX - refX, refY);
      
        path.moveTo((float)(centerX - centerRadius * 0.8), (float)(centerY + 0.4 * centerRadius));
        path.lineTo((float)(centerX - 0.25 * centerRadius), centerY + radius);

        path.moveTo((float)(centerX + centerRadius * 0.8), (float)(centerY + 0.4 * centerRadius));
        path.lineTo((float)(centerX + 0.25 * centerRadius), centerY + radius);

        path.moveTo(x1, y2);
        path.lineTo((float)(centerX - centerRadius * 0.8), (float)(centerY + 0.4 * centerRadius));

        path.moveTo(x2, y2);
        path.lineTo((float)(centerX + centerRadius * 0.8), (float)(centerY + 0.4 * centerRadius));

        canvas.drawPath(path, paint);

    }


    @Override
    public boolean onTouchEvent(MotionEvent e) {

        switch (e.getAction()) {
            case MotionEvent.ACTION_DOWN:
                startAngle = getPointAngle(e.getX(), e.getY());
                break;
            case MotionEvent.ACTION_MOVE:
                endAngle = getPointAngle(e.getX(), e.getY());
                referAngle += endAngle - startAngle;
                if (referAngle > 180) referAngle -= 360;
                else if (referAngle < -180) referAngle += 360;

                if (referAngle > 90) {
                    referAngle = 90;

                } else if (referAngle < -90) {
                    referAngle = -90;


                }

                if (lastReferAngle == -90) {
                    if (referAngle > 0) {
                        referAngle = -90;
                    }
                } else if (lastReferAngle == 90) {
                    if (referAngle < 0) {
                        referAngle = 90;
                    }
                }

                lastReferAngle = referAngle;


                if (this.touchListener != null) {
                    touchListener.onMove(this, referAngle);
                }

                break;
            default:
                break;
        }

        invalidate();
        //返回true表明处理方法已经处理该事务
        return true;

    }


    public float getAngleOffset(float angle) {
        if (angle < 90) {
            return angle + 90;
        } else {
            return angle - 270;
        }

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

