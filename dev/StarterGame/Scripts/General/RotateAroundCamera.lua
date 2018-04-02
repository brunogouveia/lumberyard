local rotatearound =
{
	Properties =
	{
		MoveSpeed = { default = 10.0 },
		TargetDistance = { default = 10 },
		TargetEntity = { default=EntityId()},
		ChangeTargetLookAngle = { default=false },
	},
}

function rotatearound:OnActivate()
	if (self.Properties.TargetEntity:IsValid()) then
		self.tickBusHandler = TickBus.Connect(self);
		
		self.currentAngle = 0.0;
	end
end

function rotatearound:OnDeactivate()		
	if(not self.tickBusHandler == nil) then
		self.tickBusHandler:Disconnect();
		self.tickBusHandler = nil;
	end
end

function rotatearound:OnTick(deltaTime, timePoint)
	--Debug.Log("Pickup graphic values:  Rotation: " .. self.rotateTimer .. " Height: " .. self.bobTimer);
	local entityTM = TransformBus.Event.GetWorldTM(self.entityId);
	local targetTM = TransformBus.Event.GetWorldTM(self.Properties.TargetEntity);
	
	self.currentAngle = self.currentAngle + deltaTime * self.Properties.MoveSpeed;
	if (self.currentAngle > 360.0) then
		self.currentAngle = self.currentAngle - 360.0;
	end
	
	local cameraTM= Transform.CreateRotationZ(Math.DegToRad(self.currentAngle));
	local cameraNewTranslation = targetTM:GetTranslation() +(cameraTM * Vector3(0.0, -self.Properties.TargetDistance, 0.0));
	cameraNewTranslation.z = entityTM:GetTranslation().z;
	cameraTM:SetTranslation(cameraNewTranslation);
	
	TransformBus.Event.SetWorldTM(self.entityId, cameraTM);
	
	-- Set Actor's look angle if enabled
	if (self.Properties.ChangeTargetLookAngle) then
		ActorComponentRequestBus.Event.SetLookAngle(self.Properties.TargetEntity, self.currentAngle - 180);
	end
end

return rotatearound;