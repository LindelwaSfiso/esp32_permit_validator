from django.contrib.auth.decorators import login_required
from django.shortcuts import render

from core.models import VerificationRecord


@login_required(login_url="admin:login")
def index(request):
	return render(request, "web/index.html", {
		'records': VerificationRecord.objects.all()[:10]
	})
