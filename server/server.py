import threading
import time

import aiohttp
from aiohttp import web, MultipartWriter
import os
import hikvisionbroker_extension
import logging
import asyncio
import rx
from rx import subject
from rx import scheduler
from rx.scheduler.eventloop import AsyncIOScheduler
from rx import operators as ops

routes = web.RouteTableDef()
camera = hikvisionbroker_extension.Camera(0)
camera.set_io_configuration()
camera.set_frame_rate(5.0)
camera.set_gain(0.0)
camera.set_exposure_time(60.0)
camera.start_acquisition()
camera.stop_acquisition()
preview_subject = subject.BehaviorSubject(None)
original_subject = subject.BehaviorSubject(None)


def parse_value(request, name='value', type_=float, raise_on_error=True):
    """
    Parse query string and get value
    :param request:
    :return:
    """
    try:
        value = type_(request.rel_url.query.get(name, ''))
    except ValueError:
        if raise_on_error:
            raise web.HTTPBadRequest(text="Failed to convert value")
        else:
            return None

    return value


@routes.post('/gain')
async def set_gain(request):
    value = parse_value(request)
    try:
        camera.set_gain(value)
    except Exception as ex:
        raise web.HTTPBadRequest(text=str(ex))
    return web.Response(text="Successfully adjusted the gain")


@routes.post('/exposure')
async def set_exposure(request):
    value = parse_value(request)
    try:
        camera.set_exposure_time(value)
    except Exception as ex:
        raise web.HTTPBadRequest(text=str(ex))
    return web.Response(text="Successfully adjusted the exposure time")


@routes.post('/fps')
async def set_frame_rate(request):
    value = parse_value(request)
    try:
        camera.set_frame_rate(value)
    except Exception as ex:
        raise web.HTTPBadRequest(text=str(ex))
    return web.Response(text="Successfully adjusted the frame rate")


@routes.post('/duration')
async def set_line_duration(request):
    value = parse_value(request, type_=int)
    try:
        camera.set_line_duration(value)
    except Exception as ex:
        raise web.HTTPBadRequest(text=str(ex))
    return web.Response(text="Successfully adjusted the line duration")


@routes.post('/preview_quality')
async def set_preview_quality(request):
    value = parse_value(request)
    try:
        camera.set_preview_quality(value)
    except Exception as ex:
        raise web.HTTPBadRequest(text=str(ex))
    return web.Response(text="Successfully adjusted the preview quality")


@routes.post('/quality')
async def set_quality(request):
    value = parse_value(request)
    try:
        camera.set_jpeg_quality(value)
    except Exception as ex:
        raise web.HTTPBadRequest(text=str(ex))
    return web.Response(text="Successfully adjusted the original quality")


@routes.post('/start')
async def start_acquisition(request):
    try:
        camera.start_acquisition()
    except Exception as ex:
        raise web.HTTPBadRequest(text=str(ex))
    return web.Response(text="Successfully started acquisition")


@routes.post('/stop')
async def stop_acquisition(request):
    try:
        camera.stop_acquisition()
    except Exception as ex:
        raise web.HTTPBadRequest(text=str(ex))
    return web.Response(text="Successfully stopped acquisition")


@routes.get('/stream')
async def stream_original(request):
    await stream(request, original_subject)

@routes.get('/preview_stream')
async def stream_preview(request):
    await stream(request, preview_subject)


async def stream(request, subject):
    boundary = 'STREAMBOUNDARY'
    response = web.StreamResponse(
        status=200,
        reason='OK',
        headers={
            'Content-Type': 'multipart/x-mixed-replace;boundary={}'.format(boundary)
        }
    )
    loop = asyncio.get_event_loop()
    queue = asyncio.queues.Queue()

    def put_queue(x):
        loop.call_soon_threadsafe(lambda: loop.create_task(queue.put(x)))

    subs = subject.subscribe(put_queue)
    await response.prepare(request)
    try:
        while True:
            frame = await queue.get()
            if frame is None:
                continue
            with MultipartWriter('image/jpeg', boundary=boundary) as mpwriter:
                mpwriter.append(frame, {
                    'Content-Type': 'image/jpeg'
                })
                await mpwriter.write(response, close_boundary=False)
    except:
        subs.dispose()


def callback(jpeg_tuple):

    preview, original = jpeg_tuple
    preview_subject.on_next(preview)
    original_subject.on_next(original)


camera.register_callback(callback)


@routes.post('/io')
async def stop_acquisition(request):
    d = dict(
        line_selector=parse_value(request, "line_selector", int, False),
        line_inverter=bool(parse_value(request, "line_inverter", int, False)),
        strobe_enable=bool(parse_value(request, "strobe_enable", int, False)),
        strobe_line_duration=parse_value(request, "strobe_line_duration", int, False),
        strobe_line_delay=parse_value(request, "strobe_line_delay", int, False)
    )
    filtered = {k: v for k, v in d.items() if v is not None}

    try:
        camera.set_io_configuration(**filtered)
    except Exception as ex:
        raise web.HTTPBadRequest(body=str(ex))
    return web.Response(body="Successfully set io configuration")


def aiohttp_server():
    app = web.Application()
    app.add_routes(routes)
    runner = web.AppRunner(app)
    return runner

loop = asyncio.new_event_loop()
def run_server(runner):
    port = os.getenv("SERVER_PORT", 8080)

    asyncio.set_event_loop(loop)
    loop.run_until_complete(runner.setup())
    site = web.TCPSite(runner, "0.0.0.0", port)
    loop.run_until_complete(site.start())
    loop.run_forever()


def main():
    t = threading.Thread(target=run_server, args=(aiohttp_server(),))
    t.setDaemon(True)
    t.start()

    try:
        t.join()
    except KeyboardInterrupt:
        print("Finalizing")
        loop.stop()
        loop.call_soon_threadsafe(lambda: loop.stop())
        t.join()
        try:
            camera.stop_acquisition()
        except:
            pass


if __name__ == '__main__':
    main()
